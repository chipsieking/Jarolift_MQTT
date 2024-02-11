"use strict";var iqwerty=iqwerty||{};iqwerty.toast=function(){function t(o,r,i){if(null!==e())t.prototype.toastQueue.push({text:o,options:r,transitions:i});else{t.prototype.Transitions=i||n;var a=r||{};a=t.prototype.mergeOptions(t.prototype.DEFAULT_SETTINGS,a),t.prototype.show(o,a),a=null}}function e(){return i}function o(t){i=t}var r=400,n={SHOW:{"-webkit-transition":"opacity "+r+"ms, -webkit-transform "+r+"ms",transition:"opacity "+r+"ms, transform "+r+"ms",opacity:"1","-webkit-transform":"translateY(-100%) translateZ(0)",transform:"translateY(-100%) translateZ(0)"},HIDE:{opacity:"0","-webkit-transform":"translateY(150%) translateZ(0)",transform:"translateY(150%) translateZ(0)"}},i=null;return t.prototype.DEFAULT_SETTINGS={style:{main:{background:"rgba(0, 0, 0, .85)","box-shadow":"0 0 10px rgba(0, 0, 0, .8)","border-radius":"3px","z-index":"99999",color:"rgba(255, 255, 255, .9)",padding:"10px 15px","max-width":"60%",width:"100%","word-break":"keep-all",margin:"0 auto","text-align":"center",position:"fixed",left:"0",right:"0",bottom:"0","-webkit-transform":"translateY(150%) translateZ(0)",transform:"translateY(150%) translateZ(0)","-webkit-filter":"blur(0)",opacity:"0"}},settings:{duration:4e3}},t.prototype.Transitions={},t.prototype.toastQueue=[],t.prototype.timeout=null,t.prototype.mergeOptions=function(e,o){var r=o;for(var n in e)r.hasOwnProperty(n)?null!==e[n]&&e[n].constructor===Object&&(r[n]=t.prototype.mergeOptions(e[n],r[n])):r[n]=e[n];return r},t.prototype.generate=function(r,n){var i=document.createElement("div");"string"==typeof r&&(r=document.createTextNode(r)),i.appendChild(r),o(i),i=null,t.prototype.stylize(e(),n)},t.prototype.stylize=function(t,e){Object.keys(e).forEach(function(o){t.style[o]=e[o]})},t.prototype.show=function(o,r){this.generate(o,r.style.main);var n=e();document.body.insertBefore(n,document.body.firstChild),n.offsetHeight,t.prototype.stylize(n,t.prototype.Transitions.SHOW),n=null,clearTimeout(t.prototype.timeout),t.prototype.timeout=setTimeout(t.prototype.hide,r.settings.duration)},t.prototype.hide=function(){var o=e();t.prototype.stylize(o,t.prototype.Transitions.HIDE),clearTimeout(t.prototype.timeout),o.addEventListener("transitionend",t.prototype.animationListener),o=null},t.prototype.animationListener=function(){e().removeEventListener("transitionend",t.prototype.animationListener),t.prototype.destroy.call(this)},t.prototype.destroy=function(){var r=e();if(document.body.removeChild(r),r=null,o(null),t.prototype.toastQueue.length>0){var n=t.prototype.toastQueue.shift();t(n.text,n.options,n.transitions),n=null}},{Toast:t}}(),"undefined"!=typeof module&&(module.exports=iqwerty.toast);

function showAllShutterChannels(){
  for (const element of document.getElementsByName('shuttercontrol')) {
    element.style.display = "block";
  }
}

function getEventLog() {
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      let log = this.responseText.replace(/(?:\r\n|\r|\n)/g, '<br />');
      document.getElementById("log").innerHTML          = log;
      document.getElementById('spinner').style.display  = "none";
      document.getElementById('log').style.display      = "block";
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=eventlog");
}

function runShutterCmd(cmd, channel, param) {
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      iqwerty.toast.Toast(this.responseText);
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=" + cmd + "&channel=" + channel + "&param=" + param);
}

function handleCheckbox(cmd, channel, param) {
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      iqwerty.toast.Toast(this.responseText);
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=" + cmd + "&channel=" + channel + "&checked=" + param);
}

function setChannelNameCallback(rowData, rowName) {
  runShutterCmd('set channel name', rowName, rowData['channel_name'])
}
let setChannelName = {"finishCallback": setChannelNameCallback};

function setShadeRuntimeCallback(rowData, rowName) {
  runShutterCmd('set shade runtime', rowName, rowData['shade_runtime'])
}
let setShadeRuntime = {"finishCallback": setShadeRuntimeCallback};

function getShutterCtrl(){
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // count configured (=named) channels
      let counter = 0;
      for (const line of this.responseText.split('\n')) {
        // line format for params: "chn=a;name=b;upDown=c;shade=d"
        let params = line.split(';');
        if (params[0] != "") {
          //get channel Id and get configured status
          const channel_id = (params[0].split('='))[1];
          let   configured = true;

          // set alternative display name if channel name is empty
          let channel_name = (params[1].split('='))[1];
          if (channel_name == ""){
            configured = false;
            channel_name = "Channel " + channel_id;
          } else {
            counter++;
          }

          // get initial checkbox value from flags
          const auto_updown_checked = ((params[2].split('='))[1] == '1') ? 'checked' : '';
          const auto_shade_checked  = ((params[3].split('='))[1] == '1') ? 'checked' : '';

          // create new element
          let element = document.createElement("div");
          element.setAttribute("name", "shuttercontrol");
          // add HTML stuff to the element
          element.innerHTML = `
            <table>
              <tr id=` + channel_id + `>
                <td style="font-size: 1.5rem;">` + channel_name + `</td>
              </tr>
            </table>
            <div class="siimple-grid-row">
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('UP',       ` + channel_id  + `)" class="siimple-btn siimple-btn--navy">UP</div></div>
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('STOP',     ` + channel_id  + `)" class="siimple-btn siimple-btn--navy">STOP</div></div>
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('DOWN',     ` + channel_id  + `)" class="siimple-btn siimple-btn--navy">DOWN</div></div>
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('SHADE',    ` + channel_id  + `)" class="siimple-btn siimple-btn--navy">SHADE</div></div>
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;"><input type="checkbox" onclick="handleCheckbox('checkAutoUpdown', ` + channel_id  + `, this.checked);" id='auto_updown'` + channel_id + ` ` + auto_updown_checked + `><label for='auto_updown'` + channel_id + `>  auto up/down</label></div></div>
              <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;"><input type="checkbox" onclick="handleCheckbox('checkAutoShade',  ` + channel_id  + `, this.checked);" id='auto_shade'`  + channel_id + ` ` + auto_shade_checked  + `><label for='auto_shade'`  + channel_id + `>  auto shade</label></div></div>
            <br><br>
          `;
          // hide element if it is not yet configured
          if (!configured){
            element.style.display = "none";
          }
          document.getElementById('container').appendChild(element);
        }
      }
      // disable spinner and show all shutter if there is no shutter configured yet.
      document.getElementById('spinner').style.display = "none";
      if (counter == 0) {
        showAllShutterChannels();
      }
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=get shutter ctrl");
}

function getShutterCfg(){
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // count configured (=named) channels
      let counter = 0;
      for (const line of this.responseText.split('\n')) {
        // line format for params: "chn=a;name=b;time=c"
        const params = line.split(';');
        if (params[0] != "") {
          //get channel Id and get configured status
          const channel_id = (params[0].split('='))[1];
          let   configured = true;

          // set alternative display name if channel name is empty
          let channel_name = (params[1].split('='))[1];
          if (channel_name == ""){
            configured = false;
            channel_name = "Channel " + channel_id;
          } else {
            counter++;
          }

          // get shuttern runtime for shading
          const runtime = (params[2].split('='))[1];

          // create new element
          let element = document.createElement("div");
          element.setAttribute("name", "shutterconfig");
          // add HTML stuff to the element
          element.innerHTML = `
            <table>
              <tr id=` + channel_id + `>
                <td>Ch-` + channel_id + ` name:</td>
                <td data-inlineType="text" data-inlineName="channel_name" style="font-size: 1.5rem;">` + channel_name + `</td>
                <td data-inlineType="doneButton">&nbsp;&nbsp;<a href="javascript:void(0)" class="siimple-link" onclick="inlineEdit('` + channel_id + `', setChannelName)">edit</a></td>
                <td>shade runtime [1/10s]:</td>
                <td data-inlineType="text" data-inlineName="shade_runtime" style="font-size: 1.5rem;">` + runtime + `</td>
                <td data-inlineType="doneButton">&nbsp;&nbsp;<a href="javascript:void(0)" class="siimple-link" onclick="inlineEdit('` + channel_id + `', setShadeRuntime)">edit</a></td>
              </tr>
            </table>
            <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('SETSHADE', ` + channel_id  + `)" class="siimple-btn siimple-btn--grey">SET SHADE</div></div>
            <div class="siimple-grid-col siimple-grid-col--2 siimple-grid-col-sm--12" align=center><div style="width: 100%; padding: 3px; margin-bottom: 2px; margin-top: 2px;" onclick="runShutterCmd('LEARN',    ` + channel_id  + `)" class="siimple-btn siimple-btn--grey">LEARN</div></div>
            <div class="siimple-grid-row"></div>
            <br><br>
          `;
          // hide element if it is not yet configured
          if (!configured){
            element.style.display = "none";
          }
          document.getElementById('container').appendChild(element);
        }
      }
      // disable spinner and show all shutters if there is no shutter configured yet.
      document.getElementById('spinner').style.display = "none";
      if (counter == 0){
        showAllShutterChannels();
      }
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=get shutter cfg");
}

function getConfig(){
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      for (const line of this.responseText.split('\n')) {
        console.log(line);
        // payload may contain '=' signs -> split at first '=' to get key first and keep payload contiguous
        let [key, val] = line.split(/=(.*)/);
        if (key != "" && val != "") {
          let params = val.split(/=(.*)/);
          if (key == "checkbox") {
            document.getElementById(params[0]).checked = (params[1] == 1);
          } else if (key == "text") {
            document.getElementById(params[0]).textContent = params[1];
          } else {
            // key is text field id -> set value from first split
            document.getElementById(key).value = val;
          }
        }
      }
      document.getElementById('spinner').style.display = "none";
      document.getElementById('config').style.display = "block";
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=get config");
}

function writeConfig(cmd){
  let params = "";
  for (const element of document.getElementById("configForm").elements) {
    if (element.type == "checkbox") {
      // produces checkbox state as "<key>=true&" or "<key>=false&"
      params += element.name + "=" + element.checked + "&";
    } else {
      params += element.name + "=" + element.value + "&"
    }
  }
  let http = new XMLHttpRequest();
  http.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      iqwerty.toast.Toast(this.responseText);
    }
  };
  http.open("POST", "api", true);
  http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  http.send("cmd=" + cmd + "&" + params);
}

var inlineEditRowContents = {};

function inlineEdit(rowName, options) {
  var tableRow = document.getElementById(rowName);
  inlineEditRowContents[rowName] = {};
  for (var i=0; i<tableRow.childElementCount; i++) {
    var cell = tableRow.children[i];
    inlineEditRowContents[rowName][i] = cell.innerHTML;
    if (options.hasOwnProperty("updateCell"))
      options.updateCell(cell, i, rowName, options);
    else
      inlineDefaultUpdateCell(cell, i, rowName, options);
  }
}

function inlineDefaultUpdateCell(cell, i, rowName, options) {
  var attributesFilter = ["inlineoptionsvalue", "inlineoptionstitle"];
  var cellContent = "";
  var key;
  if (i === 0) {
    cellContent += `<form id='${rowName}Form'></form>`;
  }
  switch (cell.dataset.inlinetype) {
    case "plain":
      cellContent += inlineEditRowContents[rowName][i];
      break;
    case "doneButton":
      cellContent += `<input type='submit' value='Finish' form='${rowName}Form'/>`;
      break;
    case "button":
      cellContent += inlineEditRowContents[rowName][i];
      break;
    case "link":
      cellContent += `<input type='text' value='${cell.innerText}' form='${rowName}Form'`;
      for (key in cell.dataset) {
        if (cell.dataset.hasOwnProperty(key) && key.substr(0, 6) == "inline" && attributesFilter.indexOf(key) == -1) {
          cellContent += ` ${key.substr(6)}='${cell.dataset[key]}'`;
        }
      }
      cellContent += "/>";
      break;
    case "text":
      cellContent += `<input type='text' value='${inlineEditRowContents[rowName][i]}' maxlength="25" form='${rowName}Form'`;
      for (key in cell.dataset) {
        if (cell.dataset.hasOwnProperty(key) && key.substr(0, 6) == "inline" && attributesFilter.indexOf(key) == -1) {
          cellContent += ` ${key.substr(6)}='${cell.dataset[key]}'`;
        }
      }
      cellContent += "/>";
      break;
    case "date":
      cellContent += `<input type='date' value='${inlineEditRowContents[rowName][i]}' form='${rowName}Form'`;
      for (key in cell.dataset) {
        if (cell.dataset.hasOwnProperty(key) && key.substr(0, 6) == "inline" && attributesFilter.indexOf(key) == -1) {
          cellContent += ` ${key.substr(6)}='${cell.dataset[key]}'`;
        }
      }
      cellContent += "/>";
      break;
    case "select":
      cellContent += "<select";
      for (key in cell.dataset) {
        if (cell.dataset.hasOwnProperty(key) && key.substr(0, 6) == "inline" && attributesFilter.indexOf(key) == -1) {
          cellContent += ` ${key.substr(6)}='${cell.dataset[key]}'`;
        }
      }
      cellContent += ">";
      var optionsTitle = JSON.parse(cell.dataset.inlineoptionstitle);
      var optionsValue = cell.dataset.hasOwnProperty("inlineoptionsvalue") ? JSON.parse(cell.dataset.inlineoptionsvalue) : [];
      for (var j=0; j<optionsTitle.length; j++) {
        cellContent += "<option ";
        cellContent += ((optionsValue.length<=j) ? "" : `value='${optionsValue[j]}'`);
        cellContent += ((inlineEditRowContents[rowName][i] == optionsTitle[j]) ? " selected='selected'" : "");
        cellContent += ">";
        cellContent += optionsTitle[j];
        cellContent += "</option>";
      }
      cellContent += "</select>";
      break;
    case "textarea":
      cellContent += `<textarea form='${rowName}Form'`;
      for (key in cell.dataset) {
        if (cell.dataset.hasOwnProperty(key) && key.substr(0, 6) == "inline" && attributesFilter.indexOf(key) == -1) {
          cellContent += ` ${key.substr(6)}='${cell.dataset[key]}'`;
        }
      }
      cellContent += ">";
      cellContent += inlineEditRowContents[rowName][i];
      cellContent += "</textarea>";
      break;
    default:
      cellContent += inlineEditRowContents[rowName][i];
      break;
  }
  cell.innerHTML = cellContent;
  if (i === 0) {
    // set the onsubmit function of the form of this row
    document.getElementById(rowName+"Form").onsubmit = function (event) {
      event.preventDefault();
      if (this.reportValidity()) {
        if (options.hasOwnProperty("finish"))
          options.finish(rowName, options);
        else
          inlineDefaultFinish(rowName, options);
      }
    };
  }
}

function inlineDefaultFinish(rowName, options) {
  var tableRow = document.getElementById(rowName);
  var rowData = {};
  for (var i=0; i<tableRow.childElementCount; i++) {
    var cell = tableRow.children[i];
    var getFromChildren = (i === 0) ? 1 : 0;
    switch (cell.dataset.inlinetype) {
      case "plain":
        break;
      case "doneButton":
        break;
      case "button":
        break;
      case "link":
        rowData[cell.dataset.inlinename] = cell.children[getFromChildren].value;
        inlineEditRowContents[rowName][i] = "<a href='"+cell.dataset.inlinelinkformat.replace("%link%", cell.children[getFromChildren].value)+"'>"+cell.children[getFromChildren].value+"</a>";
        break;
      case "text":
      case "date":
        rowData[cell.dataset.inlinename] = cell.children[getFromChildren].value;
        inlineEditRowContents[rowName][i] = cell.children[getFromChildren].value;
        break;
      case "select":
        rowData[cell.dataset.inlinename] = cell.children[getFromChildren].selectedIndex;
        rowData["_"+cell.dataset.inlinename+"Title"] = JSON.parse(cell.dataset.inlineoptionstitle)[cell.children[getFromChildren].selectedIndex];
        rowData["_"+cell.dataset.inlinename+"Value"] = JSON.parse(cell.dataset.inlineoptionsvalue)[cell.children[getFromChildren].selectedIndex];
        inlineEditRowContents[rowName][i] = JSON.parse(cell.dataset.inlineoptionstitle)[cell.children[getFromChildren].selectedIndex];
        break;
      case "textarea":
        // TODO textarea value is \n not <br/>
        rowData[cell.dataset.inlinename] = cell.children[getFromChildren].value;
        inlineEditRowContents[rowName][i] = cell.children[getFromChildren].value;
        break;
      default:
        break;
    }
  }

  // do whatever ajax magic
  if (options.hasOwnProperty("finishCallback"))
    options.finishCallback(rowData, rowName);

  for (i=0; i<tableRow.childElementCount; i++) {
    var cell = tableRow.children[i];
    if (options.hasOwnProperty("finishCell")) {
      // return true invokes the default finishCell function
      if (options.finishCell(cell, i, rowName, inlineEditRowContents[rowName][i]) === true) {
        inlineDefaultFinishCell(cell, i, rowName);
      }
    } else
      inlineDefaultFinishCell(cell, i, rowName);
  }
}

function inlineDefaultFinishCell(cell, i, rowName) {
  var cellContent = "";
  cellContent += inlineEditRowContents[rowName][i];
  cell.innerHTML = cellContent;
}
