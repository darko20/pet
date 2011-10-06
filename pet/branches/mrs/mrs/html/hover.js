function altProps() {
	this.w_y=0;
	this.w_x=0;
	this.navtxt=0;
	this.boxheight=0;
	this.boxwidth=0;
	this.ishover=false;
	this.isloaded=false;
	this.oktomove=false;
	this.op_id = 0;
	this.opacity_value = 0;
}

var AT=new altProps();

function getwindowdims(){
	AT.w_y = window.innerHeight;
	AT.w_x = window.innerWidth;
}

function writetxt(text) {
	if (AT.isloaded) {
		if (text != 0) {
			AT.oktomove=true;
			AT.ishover=true;
			AT.navtxt.innerHTML=text;
			AT.navtxt.style.visibility="visible";
			AT.navtxt.style.display="block";
			AT.boxheight=parseInt(AT.navtxt.offsetHeight);
			AT.opacity_value=0;
			AT.op_id = setInterval('incr_opacity()', 50);
		} else {
			clearInterval(AT.op_id);
			AT.navtxt.style.display="none";
			AT.navtxt.style.visibility="hidden";
			AT.navtxt.innerHTML='';
		}
	}
}

function incr_opacity() {
	if (AT.opacity_value <= 100) {
		AT.opacity_value += 7;
		AT.navtxt.style.opacity=(AT.opacity_value/100);
	} else {
		clearInterval(AT.op_id);
	}
}

function moveobj(evt) {
	if (AT.isloaded && AT.ishover && AT.oktomove) {
		newx = Math.min(AT.w_x - AT.boxwidth - 25, Math.max(2, evt.pageX - 45))
			+ window.pageXOffset;
		newy = evt.pageY + window.pageYOffset +
			((evt.pageY + AT.boxheight + 50  >= AT.w_y)? (-35 - AT.boxheight): 10);
		AT.navtxt.style.left = newx + 'px';
		AT.navtxt.style.top = newy + 'px';
	}
}

function atInitialize() {
	AT.navtxt = document.getElementById('navtxt');
	AT.boxwidth=(AT.navtxt.style.width)? parseInt(AT.navtxt.style.width) :
		parseInt(AT.navtxt.offsetWidth);
	AT.boxheight=parseInt(AT.navtxt.offsetHeight);
	getwindowdims();
	AT.isloaded=true;
}

document.onmousemove=moveobj;
window.onload = function() { atInitialize(); }
window.onresize=getwindowdims;

