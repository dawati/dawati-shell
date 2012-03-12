imports.gi.versions.Mx = '2.0';

const Mx = imports.gi.Mx;

function app_plugin () {}

app_plugin.prototype = {
  get_widget: function() {
    label = new Mx.Label({ text: "Example Plugin" });

    return label;
  }
}

var extensions = {
  'DawatiHomePluginsApp': app_plugin,
};
