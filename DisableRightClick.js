
## Javascript ##

# WORKS on Iframe

___________________footer_js 201020c OK works (Maybe-Yes in flash)___________
//PUT inside init_links()
    // (((((((((((((((((((((((((( option to disablerightclick ((((((((((
    var url_norightclick=location.search.substring(1).indexOf("norightclick");// != to -1 if we have this param
    if(url_norightclick!==-1) {
        document.oncontextmenu=new Function("console.log('main page: right-click-context menu -STOPPED');return false") ; //OK normal page Works

        console.log("norightclick-ENABLED");
        window.frames["sideframe1"].document.oncontextmenu = function(){console.log("sideframe1 :oncontextmenu right click-DISABLED"); return false;};   
        setInterval(function(){window.frames["sideframe1"].document.oncontextmenu = function(){console.log("setInterval sideframe1 :oncontextmenu right click-DISABLED"); return false;}; }, 2000);//test

    }
    // )))))))))))))))))))))))))) option to disablerightclick ))))))))

_________________________________________________________________


___________________footer_js 201020b OK works (NOT in flash)___________
// *** iframe onload="disableContextMenu();"

    // (((((((((((((((((((((((((( option to disablerightclick ((((((((((
    var url_norightclick=location.search.substring(1).indexOf("norightclick");// != to -1 if we have this param
    if(url_norightclick!==-1) {
            console.log("norightclick-ENABLED");
            window.frames["sideframe1"].document.oncontextmenu = function(){console.log("iFrame: right click-STOPPED"); return false;};
            document.oncontextmenu=new Function("console.log('main page: right-click-context menu -STOPPED');return false")   
    }
// ADOBE flash disable right click menu :<param name="menu" value="false" />
    // )))))))))))))))))))))))))) option to disablerightclick ))))))))

___________________________
<script type="text/javascript">
  function disableContextMenu()
  {
    window.frames["sideframe1"].document.oncontextmenu = function(){console.log("right click-DISABLED"); return false;};   
    // Or use this
    // document.getElementById("fraDisabled").contentWindow.document.oncontextmenu = function(){alert("No way!"); return false;};;    
  }  
</script>
<iframe name="sideframe1" src="" allowfullscreen="" height="98%" frameborder="0" width="98%"  onload="disableContextMenu();" onMyLoad="disableContextMenu();" oncontextmenu="return false" onselectstart="return false" ondragstart="return false" >

</iframe>


__________
https://cmsdk.com/javascript/how-do-i-disable-right-click-on-my-web-page.html
____________________

//Script for disabling right click on mouse
var message="Function Disabled!";
console.log(message);
function clickdsb(){
if (event.button==2){
alert(message);
return false;
}
}
function clickbsb(e){
if (document.layers||document.getElementById&&!document.all){
if (e.which==2||e.which==3){
alert(message);
return false;
}
}
}
if (document.layers){
document.captureEvents(Event.MOUSEDOWN);
document.onmousedown=clickbsb;
}
else if (document.all&&!document.getElementById){
document.onmousedown=clickdsb;
}
_______________-
//Disable right mouse click Script OK works (only on specific spot-NOT iframe -jon)
document.oncontextmenu=new Function("alert(message);return false")

________________________
(den douleuei se Iframe)
<body oncontextmenu="return false;">

___________________
<script language=JavaScript>
//Disable right mouse click Script OK works (only on specific spot-NOT iframe -jon)
var message="Function Disabled!";
function clickIE4(){
if (event.button==2){
alert(message);
return false;
 }
}
function clickNS4(e){
if (document.layers||document.getElementById&&!document.all){
if (e.which==2||e.which==3){
alert(message);
return false;
}
}
}
if (document.layers){
document.captureEvents(Event.MOUSEDOWN);
document.onmousedown=clickNS4;
}
else if (document.all&&!document.getElementById){
document.onmousedown=clickIE4;
}
document.oncontextmenu=new Function("alert(message);return false")
</script>
