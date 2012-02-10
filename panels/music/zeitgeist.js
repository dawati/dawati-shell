/* -*- mode: js2; js2-basic-offset: 4; indent-tabs-mode: nil -*-
 *
 * Copyright (C) 2010 Seif Lotfy <seif@lotfy.com>
 * Copyright (C) 2011 Siegfried-Angel Gevatter Pujals <siegfried@gevatter.com>
 * Copyright (C) 2010-2011 Collabora Ltd.
 *     Authored by: Seif Lotfy <seif@lotfy.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

const DBus = imports.dbus;
const Lang = imports.lang;
const DawatiPanel = imports.gi.DawatiPanel;
const Tracker = imports.gi.Tracker;
const Semantic = imports.semantic;


const SIG_EVENT = '(asaasay)';
const MAX_TIMESTAMP = 9999999999999;

// Number of results given by fullTextSearch; 100 is probably enough.
// Note: We can't currently increase this number to anything above 132, due to
// https://bugs.launchpad.net/zeitgeist-extensions/+bug/716503
const MAX_RESULTS = 100;

const ResultType = {
    // http://zeitgeist-project.com/docs/0.6/datamodel.html#resulttype
    // It's unfortunate to have to define these by hand; maybe if D-Bus had a way to introspect enums...
    MOST_RECENT_EVENTS                   : 0,
    LEAST_RECENT_EVENTS                  : 1,
    MOST_RECENT_SUBJECTS                 : 2,
    LEAST_RECENT_SUBJECTS                : 3,
    MOST_POPULAR_SUBJECTS                : 4,
    LEAST_POPULAR_SUBJECTS               : 5,
    MOST_POPULAR_ACTOR                   : 6,
    LEAST_POPULAR_ACTOR                  : 7,
    MOST_RECENT_ACTOR                    : 8,
    LEAST_RECENT_ACTOR                   : 9,
    MOST_RECENT_ORIGIN                   : 10,
    LEAST_RECENT_ORIGIN                  : 11,
    MOST_POPULAR_ORIGIN                  : 12,
    LEAST_POPULAR_ORIGIN                 : 13,
    OLDEST_ACTOR                         : 14,
    MOST_RECENT_SUBJECT_INTERPRETATION   : 15,
    LEAST_RECENT_SUBJECT_INTERPRETATION  : 16,
    MOST_POPULAR_SUBJECT_INTERPRETATION  : 17,
    LEAST_POPULAR_SUBJECT_INTERPRETATION : 18,
    MOST_RECENT_MIME_TYPE                : 19,
    LEAST_RECENT_MIME_TYPE               : 20,
    MOST_POPULAR_MIME_TYPE               : 21,
    LEAST_POPULAR_MIME_TYPE              : 22
};

const StorageState = {
    // http://zeitgeist-project.com/docs/0.6/datamodel.html#storagestate
    // As with ResultType, it would be nice if we could introspect enums through D-Bus
    NOT_AVAILABLE : 0,
    AVAILABLE     : 1,
    ANY           : 2
};

/* Zeitgeist Subjects (files, people, etc.) */

function Subject(uri, interpretation, manifestation, origin, mimetype, text, storage) {
    this._init(uri, interpretation, manifestation, origin, mimetype, text, storage);
};

Subject.prototype = {
    _init: function(uri, interpretation, manifestation, origin, mimetype, text, storage) {
        this.uri = uri;
        this.interpretation = interpretation;
        this.manifestation = manifestation;
        this.origin = origin;
        this.mimetype = mimetype;
        this.text = text;
        this.storage = storage;
    },
};

Subject.fromPlain = function(rawSubject) {
    return new Subject(rawSubject[0], // uri
                       rawSubject[1], // interpretation
                       rawSubject[2], // manifestation
                       rawSubject[3], // origin
                       rawSubject[4], // mimetype
                       rawSubject[5], // text
                       rawSubject[6]); // storage
};

Subject.toPlain = function(subject) {
    let rawSubject = [];
    rawSubject[0] = subject.uri;
    rawSubject[1] = subject.interpretation;
    rawSubject[2] = subject.manifestation
    rawSubject[3] = subject.origin;
    rawSubject[4] = subject.mimetype;
    rawSubject[5] = subject.text;
    rawSubject[6] = subject.storage;
    return rawSubject;
};

/* Zeitgeist Events */

function Event(interpretation, manifestation, actor, subjects, payload) {
    this._init(interpretation, manifestation, actor, subjects, payload);
};

Event.prototype = {
    _init: function(interpretation, manifestation, actor, subjects, payload) {
        this.id = 0;
        this.timestamp = 0;
        this.actor = actor;
        this.interpretation = interpretation;
        this.manifestation = manifestation;
        this.actor = actor;
        this.payload = payload;
        this.subjects = subjects;
    },
};

Event.fromPlain = function(rawEvent) {
    let subjects = rawEvent[1].map(Subject.fromPlain);
    let event = new Event(rawEvent[0][2], // interpretation
                          rawEvent[0][3], // manifestation
                          rawEvent[0][4], // actor
                          subjects, // subjects
                          rawEvent[2]);// payload
    event.id = rawEvent[0][0]; // id
    event.timestamp = parseInt(rawEvent[0][1], 10); // timestamp - it comes as a string over d-bus (yuck)
    return event;
};

Event.toPlain = function(event) {
    let rawEvent = [];
    rawEvent[0] = [];
    rawEvent[0][0] = event.id.toString();
    rawEvent[0][1] = event.timestamp.toString();
    rawEvent[0][2] = event.interpretation;
    rawEvent[0][3] = event.manifestation;
    rawEvent[0][4] = event.actor;
    rawEvent[1] = event.subjects.map(Subject.toPlain);
    rawEvent[2] = event.payload;
    return rawEvent;
};

// Zeitgeist D-Bus interface definitions. Note that most of these are
// incomplete, and only cover the methods/properties/signals that
// we're currently using.

/* Zeitgeist D-Bus Interface */

const LOG_NAME = 'org.gnome.zeitgeist.Engine';
const LOG_PATH = '/org/gnome/zeitgeist/log/activity';
const LogIface = {
    name: 'org.gnome.zeitgeist.Log',
    methods: [
        { name: 'GetEvents',
          inSignature: 'au',
          outSignature: 'a'+SIG_EVENT },
        { name: 'FindRelatedUris',
          inSignature: 'au',
          outSignature: '(xx)a(' + SIG_EVENT + ')a'+ SIG_EVENT + 'uuu' },
        { name: 'FindEventIds',
          inSignature: '(xx)a' + SIG_EVENT + 'uuu',
          outSignature: 'au' },
        { name: 'FindEvents',
          inSignature: '(xx)a' + SIG_EVENT + 'uuu',
          outSignature: 'a' + SIG_EVENT },
        { name: 'InsertEvents',
          inSignature: 'a' + SIG_EVENT,
          outSignature: 'au' },
        { name: 'DeleteEvents',
          inSignature: 'au',
          outSignature: '(xx)' },
        { name: 'DeleteLog',
          inSignature: '',
          outSignature: '' },
        { name: 'Quit',
          inSignature: '',
          outSignature: '' },
        // FIXME: Add missing DBus Methods
        // - InstallMonitor
        // - RemoveMonitor
    ],
    properties: [
        { name: 'Get',
          inSignature: 'ss',
          outSignature: 'v',
          access: 'read' },
        { name: 'Set',
          inSignature: 'ssv',
          outSignature: '',
          access: 'read' },
        { name: 'GetAll',
          inSignature: 's',
          outSignature: 'a{sv}',
          access: 'read' },
    ]
};

const Log = DBus.makeProxyClass(LogIface);
const _log = new Log(DBus.session, LOG_NAME, LOG_PATH);

function findEvents(timeRange, eventTemplates, storageState, numEvents, resultType, callback) {
    function handler(results, error) {
        if (error != null)
            log("Error querying Zeitgeist for events: "+error);
        else
            callback(results.map(Event.fromPlain));
    }
    _log.FindEventsRemote(timeRange, eventTemplates.map(Event.toPlain),
                          storageState, numEvents, resultType, handler);
}

function findEventIds(timeRange, eventTemplates, storageState, numEvents, resultType, callback) {
    function handler(results, error) {
        if (error != null)
            log("Error querying Zeitgeist for event IDs: "+error);
        else
            callback(results);
    }
    _log.FindEventIdsRemote(timeRange, eventTemplates.map(Event.toPlain),
                          storageState, numEvents, resultType, handler);
}

function deleteEvents(eventIds) {
    _log.DeleteEventsRemote(eventIds);
}

/* Zeitgeist Full-Text-Search Interface */

const INDEX_NAME = 'org.gnome.zeitgeist.Engine';
const INDEX_PATH = '/org/gnome/zeitgeist/index/activity';
const IndexIface = {
    name: 'org.gnome.zeitgeist.Index',
    methods: [
        { name: 'Search',
          inSignature: 's(xx)a'+SIG_EVENT+'uuu',
          outSignature: 'a'+SIG_EVENT+'u' },
    ],
};

const Index = DBus.makeProxyClass(IndexIface);
const _index = new Index(DBus.session, INDEX_NAME, INDEX_PATH);

/**
 * fullTextSearch:
 *
 * Asynchronously search Zeitgeist's index for events relating to the query.
 *
 * @param query The query string, using asterisks for wildcards. Wildcards must
 *        be used at the start and/or end of a string to get relevant information.
 * @param eventTemplates Zeitgeist event templates, see
 *        http://zeitgeist-project.com/docs/0.6/datamodel.html#event for more
 *        information
 * @param callback The callback, takes a list containing Zeitgeist.Event
 *        objects
 */
function fullTextSearch(query, eventTemplates, callback) {
    function handler(results, error) {
        if (error != null)
            log("Error searching with Zeitgeist FTS: "+error);
        else
            callback(results[0].map(Event.fromPlain));
    }
    _index.SearchRemote(query, [0, MAX_TIMESTAMP],
                        eventTemplates.map(Event.toPlain),
                        0, // offset into the search results
                        MAX_RESULTS,
                        ResultType.MOST_POPULAR_SUBJECTS, handler);
}

//
// Result set: Model a zeitgeist query
//

function ResultSet(sorting) {
    this._init(sorting);
}

ResultSet.prototype = {
    _init: function(sorting) {
        this.store = DawatiPanel.mpl_create_audio_store();
        this.sorting = sorting;
        this.uris = [];
        this.tracker_uris = {};
        this._init_zeitgeist();
    },


    _get_valid_uris_from_events: function (events) {
        let uris = [];
        for (let i=0; i < events.length; i++) {
            for (let j=0; j < events[i].subjects.length; j++) {
                let uri = unescape(events[i].subjects[j].uri).replace('file://', '');
                if (GLib.file_test(uri, GLib.FileTest.EXISTS))
                    uris.push("file://"+escape(uri));
                if (uris.length == MAX_RESULTS)
                    break;
            }
            if (uris.length == MAX_RESULTS)
                break;
        }
        return uris;
    },

    _init_zeitgeist_callback: function (events) {
        this.uris = this._get_valid_uris_from_events(events);
        this._init_tracker();
        // TODO: get music with the following uris
    },

    _init_zeitgeist: function () {
        if (this.sorting == Zeitgeist.ResultType.MOST_RECENT_SUBJECTS) {
            let subjTemplate = new Zeitgeist.Subject('', Semantic.NFO_AUDIO, '', '', '', '', '');
            let eventTemplate = new Zeitgeist.Event('', '', '', [subjTemplate], []);
            Zeitgeist.findEvents([new Date().getTime() - 86400000*7, Zeitgeist.MAX_TIMESTAMP],
                                 [eventTemplate],
                                 Zeitgeist.StorageState.ANY,
                                 1000,
                                 this.sorting,
                                 Lang.bind(this, this._init_zeitgeist_callback));
        }
        else if (this.sorting == Zeitgeist.ResultType.MOST_POPULAR_SUBJECTS) {
            let subjTemplate = new Zeitgeist.Subject('', Semantic.NFO_AUDIO, '', '', '', '', '');
            let eventTemplate = new Zeitgeist.Event('', '', '', [subjTemplate], []);
            Zeitgeist.findEvents([new Date().getTime() - 86400000*30, Zeitgeist.MAX_TIMESTAMP],
                                 [eventTemplate],
                                 Zeitgeist.StorageState.ANY,
                                 1000,
                                 this.sorting,
                                 Lang.bind(this, this._init_zeitgeist_callback));
        }
        else
        {
            this._init_tracker();
        }
    },

    _init_tracker: function() {
        let uris_string = "(";
        for (let i=0; i<this.uris.length; i++){
            uris_string = uris_string + "'" + this.uris[i] + "'";
            if (i<this.uris.length-1){
                uris_string = uris_string + ", ";
            }
        }
        uris_string = uris_string + ")";
        let request = "select" +
            " nie:url(?u)" +
            " nie:title(?u)" +
            " nmm:artistName(nmm:performer(?u)) " +
            " nmm:albumTitle(nmm:musicAlbum(?u))" +
            " where { ?u a nmm:MusicPiece; nie:url" +
            " ?url. FILTER (?url IN "+ uris_string +")}"
        log("Starting request : " + request);
        this.trk_connection = Tracker.SparqlConnection.get(null);
        this.trk_connection.query_async(request,
                                        null,
                                        Lang.bind(this, this._tracker_results));
    },

    _tracker_process_item: function(cursor, result) {
        if (cursor.next_finish(result)) {
            // Batching results
            if (this.data_array == null || this.data_array.length == 0)
            {
                this.data_array = new Array();
                this.data_array[this.data_array.length] = ["",
                                                           cursor.get_string(0, null)[0],
                                                           cursor.get_string(1, null)[0],
                                                           cursor.get_string(2, null)[0],
                                                           cursor.get_string(3, null)[0]];
            } else {
                if (this.data_array.length < 50) {
                    this.data_array.push(["",
                                          cursor.get_string(0, null)[0],
                                          cursor.get_string(1, null)[0],
                                          cursor.get_string(2, null)[0],
                                          cursor.get_string(3, null)[0]]);
                }
                else
                {
                    while (this.data_array.length > 1) {
                        let ldata = this.data_array.shift();

                        ldata0 = ldata[0];
                        let ldata1 = "Unknown";
                        if (ldata[1] != null && ldata[1] != "")
                            ldata1 = ldata[1];
                        let ldata2 = "Unknown";
                        if (ldata[2] != null && ldata[2] != "")
                            ldata2 = ldata[2];
                        let ldata3 = "Unknown";
                        if (ldata[3] != null && ldata[3] != "")
                            ldata3 = ldata[3];
                        let ldata4 = "Unknown";
                        if (ldata[4] != null && ldata[4] != "")
                            ldata4 = ldata[4];

                        if (this.sorting == null)
                            this.uris.push(ldata1);
                        this.tracker_uris[ldata1] = [ldata0, ldata1, ldata2, ldata3, ldata4];
                    }
                    this.data_array = new Array();
                }
            }
            cursor.next_async(null,
                              Lang.bind(this, this._tracker_process_item));
        } else {
            if (this.data_array != null) {
                while (this.data_array.length >= 1) {
                    let ldata = this.data_array.shift();

                    ldata0 = ldata[0];
                    let ldata1 = "Unknown";
                    if (ldata[1] != null && ldata[1] != "")
                        ldata1 = ldata[1];
                    let ldata2 = "Unknown";
                    if (ldata[2] != null && ldata[2] != "")
                        ldata2 = ldata[2];
                    let ldata3 = "Unknown"
                    if (ldata[3] != null && ldata[3] != "")
                        ldata3 = ldata[3];
                    let ldata4 = "Unknown"
                    if (ldata[4] != null && ldata[4] != "")
                        ldata4 = ldata[4];

                    if (this.sorting == null)
                        this.uris.push(ldata1);
                    this.tracker_uris[ldata1] = [ldata0, ldata1, ldata2, ldata3, ldata4];
                }
            }
            this.data_array = new Array();
            log("loading done.");
            for (let i=0; i < this.uris.length; i++) {
                let track = this.tracker_uris[this.uris[i]];
                if (track != undefined) {
                    let iter = this.store.append();
                    DawatiPanel.mpl_audio_store_set(this.store, iter,
                                                    track[0],
                                                    track[1],
                                                    track[2],
                                                    track[3],
                                                    track[4]);
                }
            }
        }
    },

    _tracker_results: function(connection, result) {
        let cursor = connection.query_finish(result);
        cursor.next_async(null,
                          Lang.bind(this, this._tracker_process_item));
    }
};
