const Lang = imports.lang;
const DawatiPanel = imports.gi.DawatiPanel;
const Tracker = imports.gi.Tracker;

//
// Result set: Model a tracker query
//

function ResultSet(request) {
    this._init(request);
}

ResultSet.prototype = {
    _init: function(request) {
        this.store = DawatiPanel.mpl_create_audio_store();
        this.request = request;

        this._init_tracker();
    },

    _init_tracker: function() {
        log("Starting Tracker request : " + this.request);
        this.trk_connection = Tracker.SparqlConnection.get(null);
        this.trk_connection.query_async(this.request,
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
                        let iter = this.store.append();
                        DawatiPanel.mpl_audio_store_set(this.store, iter,
                                                        ldata[0],
                                                        ldata[1],
                                                        ldata[2],
                                                        ldata[3],
                                                        ldata[4]);
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
                    let iter = this.store.append();
                    DawatiPanel.mpl_audio_store_set(this.store, iter,
                                                    ldata[0],
                                                    ldata[1],
                                                    ldata[2],
                                                    ldata[3],
                                                    ldata[4]);
                }
            }
            this.data_array = new Array();
        }
    },

    _tracker_results: function(connection, result) {
        let cursor = connection.query_finish(result);
        cursor.next_async(null,
                          Lang.bind(this, this._tracker_process_item));
    }
};
