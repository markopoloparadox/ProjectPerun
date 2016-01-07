//Ucitavanje TCP biblioteke
var net = require("net");
var fs = require("fs");
var dgram = require('dgram');

var clients = [];
var chats = [];

function PersonQuitChat(data, socket) {
    
    while(data.userlist.indexOf(socket.name)  !== -1) {
        data.userlist.splice(data.userlist.indexOf(socket.name), 1);
    }
    
    SendGroupMsg(data, socket);
}

function AddPersonToChat(data, socket) {
    var post = new Object();
    
    var person = GetClientByEmail(data.username);
    if(person == 0 || data.userlist.indexOf(data.username) != -1) {
        post.connection = "0015";
        socket.write(JSON.stringify(post));
        return;
    } else {
        post.connection = "dummy";
        socket.write(JSON.stringify(post));
    }
    
    data.msg = "Has added " + data.username + " to chat...";
    data.userlist.push(data.username);
    
    for(var index in data.userlist) {
        var name = data.userlist[index];
        var i = GetClientByEmail(name);
        if(i != -1) {
            post.connection = "0023";
            post.chatid = data.chatid;
            post.userlist = data.userlist;
            post.msg = socket.name + ":" + data.msg;
            
            var message = JSON.stringify(post);
            var client = dgram.createSocket("udp4");
            client.send(message, 0, message.length, i.port, i.remoteAddress, function(err) {
              console.log("")
            });
        }
    }
}

function SendGroupMsg(data, socket) {
    var post = new Object();
    for(var index in data.userlist) {
        var name = data.userlist[index];
        var i = GetClientByEmail(name);
        if(i != 0) {
            post.connection = "0023";
            post.chatid = data.chatid;
            post.userlist = data.userlist;
            post.msg = socket.name + ":" + data.msg;
            
            var message = JSON.stringify(post);
            var client = dgram.createSocket("udp4");
            client.send(message, 0, message.length, i.port, i.remoteAddress, function(err) {
<<<<<<< HEAD
=======
              console.log("Mislim da se nije sve poslalo?")
>>>>>>> origin/master
            });
        }
    }
}

function CreateChat(username, socket) {
    var post = new Object();
    var i = GetClientByEmail(username)
    if(i == 0) {
        post.connection = "0015";
        socket.write(JSON.stringify(post));
        return;
    }
    
    var chatElement = new Object();
    chatElement.id = chats.length + 1;
    chatElement.userlist = [socket.name, username];
    chats.push(chatElement);
    
    post.connection = "0022";
    post.chatid = chatElement.id;
    post.userlist = chatElement.userlist;
    post.msg = "";
    socket.write(JSON.stringify(post));
    
    var cli = GetClientByEmail(socket.name);
    var message = JSON.stringify(post);
    var client = dgram.createSocket("udp4");
    client.send(message, 0, message.length, cli.port, cli.remoteAddress, function(err) {
        client.close();
    });
}

function ReadDatabase() {
    var TextFromFile = fs.readFileSync(__dirname + "\\BazaKorisnika.json", "utf8");
    var users = JSON.parse(TextFromFile);
    return users;
}

function WriteToDatabase(users) {
<<<<<<< HEAD
    users = JSON.stringify(users);
=======
    users = JSON.stringify(users)
>>>>>>> origin/master
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

        WriteToDatabase(users);
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
            object.custom_status = "Online";
			object.current_game = "";
			object.ingame_start_time = new Date();
            object.index = index;
            object.port = port;

            clients.push(object);
            console.log("LOGIN: " + socket.name);
            socket.write(JSON.stringify(post));
            return;
        }
    }
}

function AddLeadingZeros(num, size) {
    var s = num+"";
<<<<<<< HEAD
    while (s.length < size)
		s = "0" + s;
=======
    while (s.length < size)		s = "0" + s;
>>>>>>> origin/master
    return s;
}

function CheckForUpdateGamesList(usr_size, usr_datetime, socket) {
    var filename = 'gameslist.dat';
	var usr_last_modified = new Date(usr_datetime);
    var stat1 = fs.statSync(filename);
    var post = new Object();
    if (stat1.mtime.getTime() > usr_last_modified.getTime() || stat1.size != usr_size) {
			post.connection = "0020";
			post.size = AddLeadingZeros(stat1.size, 8);		//defining post.size as fixed size (otherwise would number 0 (represented as integer) be converted to string "0" and 1000 to "1000" after stringifying, so packets' size and structure would not be always same!) - can contain value from 0 to 10 000 000
			var fajl = fs.readFileSync(filename, "binary");
            post.file = fajl;
			console.log("Games list is not same like latest version. File is sent to client.");
    } else {
        post.connection = "0019";
        console.log("Games list is up to date.");
    }
    socket.write(JSON.stringify(post));
	return;
}
<<<<<<< HEAD

function ChangeGameActivity(email, current_game) {		//counting user's time spent on some game
	var index = FindUserByEmail(clients, email);
	if (current_game == "") {		//if user stopped playing current game (so currently (s)he isn't playing anything)
		var now = new Date();	//current time (time when user ends playing game which (s)he played till now)
		var difference = now.getTime() - clients[index].ingame_start_time.getTime();	//in milliseconds
		var seconds_elapsed = difference/1000;
		console.log( email + " spent " + seconds_elapsed + " seconds playing " + GameNameOnly(CurrUserGame(email)) );
		
		//UpdateGameStatistics(email, GameNameOnly(CurrUserGame(email)), seconds_elapsed);
	}
	else {		//if user now started playing game
		FindUserByEmail(clients, email).ingame_start_time = new Date();
		console.log(email + " started playing " + GameNameOnly(current_game) + " on " + clients[index].ingame_start_time);
	}
}

function GameNameOnly(game_with_IP) {
	var IP_position = game_with_IP.lastIndexOf("(");	//precisely, it is position where is opened rounded bracket (which stands before IP address of game server)
	if (IP_position != -1)
		game_with_IP = game_with_IP.slice(0, IP_position-1);
	return game_with_IP;	//which is now without IP address (if there wasn't IP address, then is original string returned)
}

function StatusChanged(email, custom_status, current_game) {
	var index = FindUserByEmail(clients, email);
	if (GameNameOnly(clients[index].current_game) != GameNameOnly(current_game))	//if user stopped playing game which (s)he played before
		ChangeGameActivity(email, current_game);
	clients[index].custom_status = custom_status;
	clients[index].current_game = current_game;
}
=======
>>>>>>> origin/master

function CurrUserStatus(email) {
    for(var i = 0; i < clients.length; ++i)
        if(clients[i].email == email)
            return clients[i].custom_status;

    return "Offline";
}

<<<<<<< HEAD
function CurrUserGame(email) {
	for(var i = 0 ; i < clients.length; ++i)
		if(clients[i].email == email)
			return clients[i].current_game;
	return "";		//if user isn't playing anything, then return empty string
}

/*function RequestForGameStatistics (email, socket) {
	post = new Object();
	post.connection = "0028";
//	if (FindUserByEmail(clients, email)==-1) {
//		post.stats = [];
//		console.log(email + " sent request for game statistics for unexisted user.");
//	}
//	else {		//if forwarded email (username) exists in database
//	post.stats = ReadGameStatistics(email);			//opet otkomentirati
//	socket.write(JSON.stringify(post));				//opet otkomentirati
//	}
}*/

/*function ReadGameStatistics(email) {
	fs.readFileSync(__dirname + "\\GameActivity.json", "utf8");
	if (fs.size==0) {
		
	}
/*    var TextFromFile = fs.readFileSync(__dirname + "\\GameActivity.json", "utf8");
    var all_users = JSON.parse(TextFromFile);
	for (var i=0 ; i<all_users.length ; i++)
		if (all_users[i].user == email)
			return all_users[i];
	return [];		//if for requested username is still no game statistics saved
}*/

/*function UpdateGameStatistics(user, game, time_played) {
	var TextFromFile = fs.readFileSync(__dirname + "\\GameActivity.json", "utf8");		//mozda nejde Sync???
    var content = JSON.parse(TextFromFile);
	var user_exists = false;
	var game_exists = false;
	for (var i=0 ; i<content.length ; i++)
		if (content[i].user == user) {
			user_exists = true;
			for (var j=0 ; j<content[i].stats.length ; j++) {
				if (content[i].stats[j].game == game) {
					game_exists = true;
					content[i].stats[j].time_played += time_played;		//maybe this needs to be changed into integer (mislim da ipak nejde int)
				}
			}
			if (game_exists == false)
				content[game] = time_played;
				//content[i].stats.push([game, time_played]);		//not sure if 'key':'val' pair must be sent as an array
		}
	if (user_exists == false) {
		var stat = {
			game: time_played
		};
//		user = [];
//		user.push(stat);
		var user = new Object();
		user.game = game;
		user.time_played = time_played;
		content.push(user);
//		content[user]=[];
//		content[user].push(stat);
//		content[user].stats.push(stat);
		//content[user].push(stat);	//not sure if this will work
//		content[user][game] = time_played;
//		content[user][stat];
	}
    fs.writeFileSync(__dirname + "\\GameActivity.json", JSON.stringify(content), function (err) {
        if (err)	return console.log(err);
    });
}*/

=======
>>>>>>> origin/master
function AddFriendToFriendList(users, index, socket, email) {
    for(var i = 0; i < users[index].friends.length; ++i)
        if(users[index].friends[i] == email)
            return 0;

    users[index].friends.push(email);
    WriteToDatabase(users);
    console.log("Dodan je prijatelj: " + socket.name + "->" + email);
    return 1;
}

function AddFriendUser(email, socket) {
    var post = new Object();

    if(socket.name == email) {
        var post = new Object();
        post.connection = "0013";
        socket.write(JSON.stringify(post));
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
        object.custom_status = "Offline";
		object.current_game = "";
        object.email = friends[i];
        for(var j = 0; j < clients.length; ++j) {
            if(friends[i] == clients[j].email) {
                object.custom_status = clients[j].custom_status;
				object.current_game = clients[j].current_game;
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

        else if(data.connection == "0004")
            LoginUser(data.email, data.password, data.port, socket);

        else if(data.connection == "0008")
            AddFriendUser(data.email, socket);

        else if(data.connection == "0011")
            SendFriendsStatus(socket);
        
        else if(data.connection == "0014")
            SendClientData(socket, data.email);
            
<<<<<<< HEAD
		else if(data.connection == "0018")
			CheckForUpdateGamesList(data.size, data.datetime, socket);
            
        else if(data.connection == "0021")
			CreateChat(data.username, socket);
            
        else if(data.connection == "0023")
			SendGroupMsg(data, socket);
        
        else if(data.connection == "0024")
            AddPersonToChat(data, socket);
        
        else if(data.connection == "0025")
            PersonQuitChat(data, socket);
		
		else if(data.connection == "0026")
			StatusChanged(socket.name, data.custom_status, data.current_game);
		
		//else if(data.connection == "0027")
			//RequestForGameStatistics (data.email, socket);
=======
		if(data.connection == "0018")
			CheckForUpdateGamesList(data.size, data.datetime, socket);
            
        if(data.connection == "0021")
			CreateChat(data.username, socket);
            
        if(data.connection == "0023")
			SendGroupMsg(data, socket);
        
        if(data.connection == "0024")
            AddPersonToChat(data, socket);
        
        if(data.connection == "0025")
            PersonQuitChat(data, socket);
>>>>>>> origin/master
    });


    socket.on('close', function() {
        console.log(socket.name + " left.\n");
        for(var i = clients.length - 1; i >= 0; --i) {
            if(clients[i]["email"] === socket.name) {
				if (CurrUserGame(socket.name) != "")		//if user exited application while (s)he was still in game
					ChangeGameActivity(socket.name,"");	//set that (s)he stopped playing game (because we can't track further game activity if (s)he is offline)
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