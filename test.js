var splitter = require("./build/default/binding");
var hello = new splitter.Splitter();

hello.appendSplitPoint("0.2");
hello.appendSplitPoint("0.4");
hello.setSplitPath("dataa");
hello.split("red.mp3",function(name,err){
	console.log(name);
	console.log(err);
});

