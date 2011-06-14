
var _button;

var _text = "Jack and Jill";
var _textBox;
var _textBoxStatus;

var _selection;
var _selectStatus;

var _setIntervalStatus;

function buttonClicked() {
    _button.value = "Clicked!!";
}

function textEntered() {
    if (_textBox.value == _text) {
        _textBoxStatus.innerHTML = "Text matches!!";
    } else {
        _textBoxStatus.innerHTML = "Text doesn't match";
    }
}

function optionSelected() {
    var s = _selection.options[_selection.selectedIndex].innerHTML;
    _selectStatus.innerHTML = "You selected " + s;
}

function setIntervalClicked() {
    var id;
    
    var interval = 10; // ms
    var ntimes = 0;
    var maxtimes = 10;
    var prevTime = (new Date()).getTime();
    
    function cb() {
        var msg = "setInterval: " + ntimes + " : expected " + interval;
        var currTime = (new Date()).getTime();
        msg += " : got " + (currTime - prevTime) + "<br>";
        _setIntervalStatus.innerHTML += msg;
        
        ntimes++;
        if (ntimes == maxtimes) {
            clearInterval(id);
            
            // Do Math.random test
            var rand = Math.random();
            msg = "Math.random() = " + rand + "<br>";
            _setIntervalStatus.innerHTML += msg;
        }
        prevTime = currTime;
    }
    
    id = setInterval(cb, interval);
}

function init() {
    var html = "";
    
    // Button test
    html += "<div><input type='submit' value='Click me' id='buttontest' onclick='buttonClicked();'/></div>\n";
    
    // Text box test
    html += "<div><input type='text' value='' id='texttest' onchange='textEntered();'/></div>\n";
    html += "<div>Enter the text '" + _text + "' without quotes</div>\n";
    html += "<div id='textstatus'>Text doesn't match</div>\n";
    
    // Selection test
    html += "<div><select id='selecttest' onchange='optionSelected();'>\n";
    html += "<option value='V'>Violet</option>\n";
    html += "<option value='I'>Indigo</option>\n";
    html += "<option value='B' selected>Blue</option>\n";
    html += "<option value='G'>Green</option>\n";
    html += "<option value='Y'>Yellow</option>\n";
    html += "<option value='O'>Orange</option>\n";
    html += "<option value='R'>Red</option>\n";
    html += "</select></div>\n";
    html += "<div id='selectstatus'></div>\n";
    
    // setInterval and Math.random tests
    html += "<div><input type='submit' value='setInterval test' onclick='setIntervalClicked();'/></div>\n";
    html += "<div id='setintervalstatus'></div>\n";
    
    // iframe test -- do it only in main window
    if (window.top == window) {
        html += "<div><iframe id='testframe'>This is an iframe</iframe></div>\n";
    }
    
    // HTML link
    html += "<a href='http://pdos.csail.mit.edu/~rameshch/replay/test-iframe.html'>Test link</a>\n";
    
    // Set innerHTML of document body
    var contentDiv = document.getElementById('content');
    contentDiv.innerHTML = html;
    
    // Get the DOM elements and assign to global vars for the convenience of
    // other functions
    _button = document.getElementById('buttontest');
    _textBox = document.getElementById('texttest');
    _textBoxStatus = document.getElementById('textstatus');
    _selection = document.getElementById('selecttest');
    _selectStatus = document.getElementById('selectstatus');
    _setIntervalStatus = document.getElementById('setintervalstatus');
    
    // set location of test iframe
    if (window.top == window) {
        var testframe = document.getElementById('testframe');
        testframe.contentWindow.location = window.location;
    }
}

init();