console.log("start version replace");
var fs = require("fs");
var path = require('path');
var filePath = path.normalize(__dirname + '/../src/YouMeIMEngine/YouMeIMCommonDef.h');
// 同步读取
console.log("open version file:" + filePath);
var data = fs.readFileSync(filePath).toString();
var re = /SDK_VER (\d+)/g;
var matchRet = re.exec(data);
console.log("pre version:" + matchRet[1]);
var preVersion = parseInt(matchRet[1]);
var currentVersion = preVersion + 1;
var newVersionFileData = data.replace(/SDK_VER (\d+)/g, "SDK_VER " + currentVersion);

console.log("new version is:" + currentVersion);
fs.writeFile(filePath, newVersionFileData, function (err) {
    if (err) {
        return console.error("upgrade build version fail:" + err);
    }
    console.log("upgrade build version success.");
});

console.log("upgrade done.");