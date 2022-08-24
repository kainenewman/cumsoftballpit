// ==UserScript==
// @name         4chanimgfix
// @namespace    Hiroshima Nagasaki
// @author       Anonymous
// @match        *://boards.4chan.org/*
// @grant        none
// ==/UserScript==
 
var a_scan = document.getElementsByTagName('a');
var hiroshima, nagasaki;
for(var i = 0; i < a_scan.length; i++) {
  hiroshima = a_scan[i];
  nagasaki = hiroshima.getAttribute('href');
  if(nagasaki) hiroshima.setAttribute('href', nagasaki.replace(/is.4chan.org/gi, 'i.4cdn.org'));
}
