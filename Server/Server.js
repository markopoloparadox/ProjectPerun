//Ucitavanje TCP biblioteke
var net = require("net");
var fs = require("fs");

var clients = [];

function AddUserToDataBase(email, password, socket) {
    var TextFromFile = "";
    TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    
    if(TextFromFile.length == 0) {
        return;
        
    } else {
        users = JSON.parse(TextFromFile);
        
        for(var i = 0; i < users.length; i++) {
            var obj = users[i];
            if(obj.email == email) {
                var post = new Object();
                post.connection = "0002";
                
                socket.write(JSON.stringify(post));
                return;
            }
        }
        
        var post = new Object();
        post.connection = "0003";
        
        socket.write(JSON.stringify(post));
        
        var friends = [];
        
        var user = new Object();
        user.email = email;
        user.password = password;
        user.friends = friends;
        users.push(user);
    }
    
    users = JSON.stringify(users)
    
    fs.writeFile(__dirname + "\\BazaKorisnika.json", users, function (err) {
        if (err) return console.log(err);
        console.log("Dodan je korisnik: " + user.email + " " + user.password);
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
            if((obj.email == email) && (obj.password != password)) {
                var post = new Object();
                post.connection  = "0006";
                
                socket.write(JSON.stringify(post));
                return;
            }
            if((obj.email == email) && (obj.password == password)) {
                var post = new Object();
                post.connection  = "0007";
                socket.name = email;
                
                var object = new Object;
                object.email = socket.name;
                object.remoteAddress = socket.remoteAddress;
                object.remotePort = socket.remotePort;
                object.status = "Online";
                object.index = i;
                
                clients.push(object);
                
                socket.write(JSON.stringify(post));
                return;
            }
        }
    }
    var post = new Object();
    post.connection = "0005";
    
    socket.write(JSON.stringify(post));
    return;
}


function CurrUserStatus(email) {
    for(var i = 0; i < clients.length; ++i)
        if(clients[i].email == email)
            return clients[i].status;
    
    return "Offline";
}


function AddFriendToFriendList(users, socket, email) {
    for(var i = 0; i < users.length; ++i) {
        var obj = users[i];
        if(obj.email == socket.name) {
            
            for(var j = 0; j < obj.friends.length; ++j) {
                if(obj.friends[j] == email) {
                    return 0;
                }
            }
            
            users[i].friends.push(email);
            users = JSON.stringify(users)
            fs.writeFile(__dirname + "\\BazaKorisnika.json", users, function (err) {
                if (err) return console.log(err);
                console.log("Dodan je prijatelj: " + socket.name + "->" + email);
            });
            return 1;
        }
    }
}

function AddFriendUser(email, socket) {
    if(socket.name == email) {
        var post = new Object();
        post.connection = "0013";
        socket.write(JSON.stringify(post))
        return;
    }
    
    
    var TextFromFile = "";
    
    TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    
    if(TextFromFile.length == 0) {
        return;
        
    } else {
        users = JSON.parse(TextFromFile);
        for(var i = 0; i < users.length; ++i) {
            var obj = users[i];
            if((obj.email == email)) {
                
                if(AddFriendToFriendList(users, socket, email) == 0) {
                    var post = new Object();
                    post.connection = "0010";
                    
                    socket.write(JSON.stringify(post));
                    return;
                }
                
                var post = new Object();
                post.connection = "0009";
                socket.write(JSON.stringify(post));
                
                return;
            }
        }
    }
    var post = new Object();
    post.connection = "0005";
    socket.write(JSON.stringify(post))
    return;
}

function SendFriendsStatus(socket) {
    var TextFromFile = "";
    
    TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    
    if(TextFromFile.length == 0) {
        return;
        
    } else {
        users = JSON.parse(TextFromFile);
        for(var i = 0; i < users.length; ++i) {
            var obj = users[i];
            if((obj.email == socket.name)) {
                var AllFriends = [];
                
                var friends = obj.friends;
                
                for(var j = 0; j < friends.length; ++j) {
                    var object = new Object();
                    object.status = "Offline";
                    object.email = friends[j];
                    for(var k = 0; k < clients; ++k) {
                        if(friends[j] == clients[k].email) {
                            object.status = clients[k].status;
                            break;
                        }
                    }
                    AllFriends.push(object);
                }
                
                var post = new Object();
                post.connection = "0009";
                post.friends = AllFriends;
                socket.write(JSON.stringify(post));
                return;
            }
        }
    }
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
            
        if(data.connection == "0008")
            AddFriendUser(data.email, socket);
        
        if(data.connection == "0011")
            SendFriendsStatus(socket);
    });
    
    
    socket.on('close', function() {
        console.log(socket.name + " left.\n");
        for(var i = clients.length - 1; i >= 0; --i) {
            if(clients[i]["name"] === socket.name) {
                clients.splice(i, 1);
                break;
            }
        }
    });
    
    socket.on('error', function(err){
        console.log("Error");
    });
    
    
});
server.listen(1337, '127.0.0.1');