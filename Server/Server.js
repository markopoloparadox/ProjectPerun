//Ucitavanje TCP biblioteke
var net = require("net");
var fs = require("fs");

var clients = [];

function ReadDatabase() {
    var TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    var users = JSON.parse(TextFromFile);
    return users;
}

function WriteToDatabse(users) {
    users = JSON.stringify(users)
    fs.writeFile(__dirname + "\\BazaKorisnika.json", users, function (err) {
        if (err) return console.log(err);
    });
}

function FindUserByEmail(users, email) {
    for(var i = 0; i < users.length; ++i) {
        if(users[i].email == email)
            return i;
    }
    return -1;
}

function AddUserToDataBase(email, password, socket) {

    var users = ReadDatabase();
    var index = FindUserByEmail(users, email)
    if(index != -1) {
        var post = new Object();
        post.connection = "0002";

        socket.write(JSON.stringify(post));
    } else {
        var friends = [];
        var user = new Object();
        user.email = email;
        user.password = password;
        user.friends = friends;
        users.push(user);

        WriteToDatabse(users);
        console.log("Dodan je korisnik: " + user.email + " " + user.password);

        var post = new Object();
        post.connection = "0003";
        socket.write(JSON.stringify(post));
    }
}

function LoginUser(email, password, port, socket) {

    var users = ReadDatabase();
    var index = FindUserByEmail(users, email)
    var post = new Object();

    if(index == -1) {
        post.connection = "0005";
        socket.write(JSON.stringify(post));
        return;
    } else {
        if(users[index].password != password) {
            post.connection  = "0006";

            socket.write(JSON.stringify(post));
            return;
        } else {
            post.connection  = "0007";
            socket.name = email;

            var object = new Object;
            object.email = socket.name;
            object.remoteAddress = socket.remoteAddress;
            object.remotePort = socket.remotePort;
            object.status = "Online";
            object.index = index;
            object.port = port;

            clients.push(object);
            console.log("LOGIN: " + socket.name);
            socket.write(JSON.stringify(post));
            return;
        }
    }
}


function CurrUserStatus(email) {
    for(var i = 0; i < clients.length; ++i)
        if(clients[i].email == email)
            return clients[i].status;

    return "Offline";
}


function AddFriendToFriendList(users, index, socket, email) {
    for(var i = 0; i < users[index].friends.length; ++i)
        if(users[index].friends[i] == email)
            return 0;

    users[index].friends.push(email);
    WriteToDatabse(users);
    console.log("Dodan je prijatelj: " + socket.name + "->" + email);
    return 1;
}

function AddFriendUser(email, socket) {
    var post = new Object();

    if(socket.name == email) {
        var post = new Object();
        post.connection = "0013";
        socket.write(JSON.stringify(post))
        return;
    }

    var users = ReadDatabase();
    var index = FindUserByEmail(users, socket.name);


    if(index == -1) {
        post.connection = "0005";
        socket.write(JSON.stringify(post))
        return;
    } else {
        if(AddFriendToFriendList(users, index, socket, email) == 0) {
            post.connection = "0010";
            socket.write(JSON.stringify(post));
            return;
        } else {
            post.connection = "0009";
            socket.write(JSON.stringify(post));
            return;
        }
    }
}

function SendFriendsStatus(socket) {
    var post = new Object();
    var users = ReadDatabase();
    var index = FindUserByEmail(users, socket.name);

    var AllFriends = [];
    var friends = users[index].friends;

    for(var i = 0; i < friends.length; ++i) {
        var object = new Object();
        object.status = "Offline";
        object.email = friends[i];
        for(var j = 0; j < clients.length; ++j) {
            if(friends[i] == clients[j].email) {
                object.status = clients[j].status;
                break;
            }
        }
        AllFriends.push(object);

    }
    post.connection = "0012";
    post.friends = AllFriends;
    socket.write(JSON.stringify(post));
    return;
}


function GetClientByEmail(email) {
    for(var i = 0; i < clients.length; ++i)
        if(clients[i].email == email)
            return clients[i];
    return 0;
}

function SendClientData(socket, email) {
    var outClient = GetClientByEmail(email);
    
    var post = new Object();
    if(outClient == 0) {
        post.connection = "0015";
        socket.write(JSON.stringify(post));
        return;
    }
    
    post.connection = "0016";
    post.ip = outClient.remoteAddress;
    post.port = outClient.port;
    post.clname = socket.name;
    socket.write(JSON.stringify(post));
    console.log(post);
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
            LoginUser(data.email, data.password, data.port, socket);

        if(data.connection == "0008")
            AddFriendUser(data.email, socket);

        if(data.connection == "0011")
            SendFriendsStatus(socket);
        
        if(data.connection == "0014")
            SendClientData(socket, data.email);
    });


    socket.on('close', function() {
        console.log(socket.name + " left.\n");
        for(var i = clients.length - 1; i >= 0; --i) {
            if(clients[i]["email"] === socket.name) {
                clients.splice(i, 1);
                break;
            }
        }
    });

    socket.on('error', function(err) {
        console.log("Error");
    });

});
server.listen(1337, '127.0.0.1');