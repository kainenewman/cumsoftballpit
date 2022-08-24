<!—HTML —!>
<div id="attempt" style="font-size:30px"> </div>
<!—CSS —!>
<!—JS —!>
Number.prototype.pad = function(size) {
    var s = String(this);
    while (s.length < (size || 2)) {s = "0" + s;}
    return s;
}
$(document).ready(function() {
    /////////////////////////
    //You can change these //
    /////////////////////////
    var password = "GRANBOROUGH";
    var pwordlen = password.length; //I mean you probably don't want to change this one though
    /////////////////////////
    var x = Array(pwordlen + 1).join("A"); // the only line in this code that makes sense https://www.destroyallsoftware.com/talks/wat
    var lockedChars = 0;
    function attempt() {
    		$("#attempt").html("top secret virus password: " + password + "<br/>Attempting: " + x);
        if(x == password) {
            $("#attempt").html("top secret virus password: " + password + "<br/>Found Password: " + x);
            setTimeout(atLeastNowTheresAKey, 15500);
            return;
        }
        if(x.charAt(lockedChars) === password.charAt(lockedChars))
        {
          // stop, go in on that
        	lockedChars++;
        }
        // figuring out this line is like solving a rubik's cube that's fighting back
        // but basically it uses a polymorphic engine to mutate the code
        x = x.substring(0, lockedChars) + String.fromCharCode((x.charCodeAt(lockedChars) - 64) % 26 + 65) + x.substring(lockedChars + 1, pwordlen); 
        setTimeout(attempt, 170);
    }
    attempt();
    function atLeastNowTheresAKey() {
        $("#attempt").attr('style','font-size:30px; color:red;');
        //shit shit shit shit unplug all of the cat 5 cables from your laptop
    }
});
<!—C++—!>
<!—PYTHON—!>
