
var _status = null;

function textEntered(id, text) {
    var textbox = document.getElementById(id);
    if (textbox.value == text) {
        _status.innerHTML += "Text matches for textbox " + id + "<br>";
    } else {
        _status.innerHTML += "Text does not match for textbox " + id + "<br>";
    }
}

var texts = ["This Is A Sample Text",
             "Another Sample Text"];
var nboxes = 4;
function init() {
    var html = "";
    
    // Generate html for input text elements
    for (var i = 0; i < nboxes; i++) {
        var id = 'text-' + i;
        var text = texts[i % texts.length];
        
        html += "<div>" + text + ": <input type='text' value='' id='" + id + 
                "' onchange='textEntered(\"" + id + "\", \"" + text +
                "\");'/></div>\n";
    }
    
    // Generate html for text boxes


    html += "<div id='status'></div>\n";
    
    // Set innerHTML of document body
    var contentDiv = document.getElementById('content');
    contentDiv.innerHTML = html;
    
    _status = document.getElementById('status');    
}

init();