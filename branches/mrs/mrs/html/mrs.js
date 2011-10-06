var mrsVariables = new Object();
mrsVariables.size = 0;
mrsVariables.secondary = null;
var mrsHCONSsForward = new Object();
var mrsHCONSsBackward = new Object();

function mrsVariableSelect(name, text) {
  writetxt(text);
  classSetColor('mrsVariable' + name, 'red');
  if(mrsHCONSsForward[name]) {
    mrsVariables.secondary = mrsHCONSsForward[name];
    classSetColor('mrsVariable' + mrsHCONSsForward[name], 'green');
  } else if(mrsHCONSsBackward[name]) {
    mrsVariables.secondary = mrsHCONSsBackward[name];
    classSetColor('mrsVariable' + mrsHCONSsBackward[name], 'green');
  } // if
} // mrsVariableSelect()

function mrsVariableUnselect(name) {
  writetxt(0);
  classSetColor('mrsVariable' + name, '#1a04a5');
  if (mrsVariables.secondary) {
    classSetColor('mrsVariable' + mrsVariables.secondary, '#1a04a5');
    mrsVariables.secondary = null;
  } // if
} // mrsVariableUnselect()

function classSetColor(name, color) {
  if(mrsVariables[name] != null) {
    for (var i = 0; i < mrsVariables[name].length; ++i) 
      if(color == 'swap') {
        var foo = mrsVariables[name][i].style.color;
        mrsVariables[name][i].style.color 
          = mrsVariables[name][i].style.background;
        mrsVariables[name][i].style.background = foo;
      } else {
        mrsVariables[name][i].style.color = color;
      } // else
  } else {
    var all = 
      (document.all ? document.all : document.getElementsByTagName('*'));
    for (var i = 0; i < all.length; ++i) {
      var index = all[i].className;
      if(mrsVariables[index] == null) {
        mrsVariables[index] = new Array();
        ++mrsVariables.size;
      } //if 
      mrsVariables[index].push(all[i]);
      if(all[i].className == name) {
        if(color == 'swap') {
          var foo = all[i].style.color;
          all[i].style.color = all[i].style.background;
          all[i].style.background = foo;
        } else {
          all[i].style.color = color;
        } // else
      } // if
    } // for
  } // else
} // classSetColor()
