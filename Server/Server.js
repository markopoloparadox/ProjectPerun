//Ucitavanje TCP biblioteke
var net = require("net");
var fs = require("fs");

function AddUserToDataBase(email, password, socket) {
    var TextFromFile = "";
    TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    
    if(TextFromFile.length == 0) {
        return;
        
    } else {
        users = JSON.parse(TextFromFile);
        
        for(var i = 0; i < users.length; i++) {
            var obj = users[i];
            if(obj.m_email == email) {
                var post = new Object();
                post["connection"] = "0002";
                
                socket.write(JSON.stringify(post));
                socket.pipe(socket);
                return;
            }
        }
        
        var post = new Object();
        post["connection"] = "0003";
        
        socket.write(JSON.stringify(post));
        socket.pipe(socket);
        
        var user = new Object();
        user.m_email = email;
        user.m_password = password;
        users.push(user);
    }
    
    users = JSON.stringify(users)
    
    fs.writeFile(__dirname + "\\BazaKorisnika.json", users, function (err) {
        if (err) return console.log(err);
        console.log("Dodan je korisnik: " + user.m_email + " " + user.m_password);
    });
}

function LoginUser(email, password, socket) {
    var TextFromFile = "";
    
    TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    
    if(TextFromFile.length == 0) {
        return;
        
    } else {
        users = JSON.parse(TextFromFile);
        
        for(var i = 0; i < users.length; i++) {
            var obj = users[i];
            if((obj.m_email == email) && (obj.m_password != password)) {
                var post = new Object();
                post["connection"] = "0006";
                
                socket.write(JSON.stringify(post));
                socket.pipe(socket);
                return;
            }
            if((obj.m_email == email) && (obj.m_password == password)) {
                var post = new Object();
                post["connection"] = "0007";
                
                socket.name += ":" + email;
                
                socket.write(JSON.stringify(post));
                socket.pipe(socket);
                return;
            }
        }
    }
    var post = new Object();
    post["connection"] = "0005";
    
    socket.write(JSON.stringify(post));
    socket.pipe(socket);
    return;
}

var server = net.createServer(function(socket) {
    
    //Preponavanje klijenta
    socket.name = socket.remoteAddress + ":" + socket.remotePort;
    
    //dodavanje klijenta u listu
    
    socket.on('data', function(data) {
        data = JSON.parse(data);
        if(data.connection == "0001")
            AddUserToDataBase(data.email, data.password, socket);
        
        if(data.connection == "0004")
            LoginUser(data.email, data.password, socket);
    });
    
    
    socket.on('close', function() {
        console.log(socket.name + " left.\n");
    });
    
    socket.on('error', function(err){
        console.log("Error");
    });
    
    
});
server.listen(1337, '127.0.0.1');