Javascript User Data Display "Browser Detection"

var result = bowser.getParser(window.navigator.userAgent);
console.log(result.parsedResult.browser);
console.log(result.parsedResult.os);



var isValidBrowser = result.satisfies({
  ie: ">=11",
  firefox: ">45",
  edge: ">15"
});
console.log("isValid:", !!isValidBrowser)
