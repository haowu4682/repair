
var _status = null;

function buttonClicked(btnstr) {
    var btnid = btnstr.split('-')[1];
    
    var s = _status.innerHTML;
    if (s != "") {
        s += ", ";
    }
    s += btnid;
    _status.innerHTML = s;
}

var nbuttons = 16;
function init() {
    var html = "";

    // Generate html for buttons    
    html += "<div>Click these buttons in any order</div>\n";
    for (var i = 0; i < nbuttons; i++) {
        var btnstr = "Button-" + i;
        html += "<div><input type='submit' value='" + btnstr + "' id='" + btnstr
                + "' onclick='buttonClicked(\"" + btnstr + "\");'/></div>\n";
    }
    html += "<div id='status'></div>\n";
    
    // Set innerHTML of document body
    var contentDiv = document.getElementById('content');
    contentDiv.innerHTML = html;

    _status = document.getElementById('status');
}

init();