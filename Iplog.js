<script>
function getIP(json) {
  alert("My public IP address is: " + json.ip);
}
</script>
<script src="https://api.ipify.org?format=jsonp&callback=getIP"></script>

// as seperate script tag
<script type="application/javascript" src="https://api.ipify.org?format=jsonp&callback=getIP"> </script>
