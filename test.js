var splitter = require("./build/default/binding");
var hello = new splitter.Splitter();
hello.split("file",function(){
	console.log("bunga");
});
//console.log("ciccio");
