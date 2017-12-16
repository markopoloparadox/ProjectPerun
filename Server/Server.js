//Ucitavanje TCP biblioteke
var ip = require("ip");
var net = require("net");
var fs = require("fs");
var dgram = require('dgram');
var passwordHash = require('password-hash');

var clients = [];
var chats = [];

function NotifyChatRoomsAboutUserLogout(email) {
    chats.forEach(function(users, index) {
        var userPositionInChat = users.indexOf(email);
        if (userPositionInChat !== -1) {
            var chatGroupToModify = chats[index];
            chatGroupToModify.splice(userPositionInChat, 1);
            
            post.connection = "0022";
            post.chatid = index;
            post.userlist = chatGroupToModify;
            post.msg = email + " has left... (went offline)";
        
            SendDataToMultipleUsers(chatGroupToModify, post);
        }
    });
}

function PersonQuitChat(data, socket) {
    var chatGroupToModify = chats[data.chatid];
    chatGroupToModify.splice(chatGroupToModify.indexOf(socket.name));
    
    post.connection = "0022";
    post.chatid = data.chatid;
    post.userlist = chatGroupToModify;
    post.msg = socket.name + " has left... (went offline)";

    SendDataToMultipleUsers(chatGroupToModify, post);
}

function AddPersonsToChat(data, socket) {
    var chatGroupToModify = chats[data.chatid];
    
    data.userlist.forEach(function(username) {
        var user = GetClientByEmail(username);
        if (user) {
            chatGroupToModify.push(user);
        }
    });
    
    data.msg = socket.name + " has added " + data.usernames + " to chat...";
    
    var post = new Object();
    post.connection = "0022";
    post.chatid = data.chatid;
    post.userlist = chatGroupToModify;
    post.msg = data.msg;

    SendDataToMultipleUsers(chatGroupToModify, post);
}

function SendMsg(data, socket) {
    var post = new Object();

    post.connection = "0023";
    post.isprivate = data.isprivate;
    post.msg = socket.name + ": " + data.msg;
    if (data.isprivate) {
        post.chatid = data.chatid;
        var userIndex = FindUserByEmail(clients, socket.name);
        if (userIndex !== -1) {
            clients[userIndex].socket.write(JSON.stringify(post));
        }
        post.chatid = socket.name;
        userIndex = FindUserByEmail(clients, data.chatid);
        if (userIndex !== -1) {
            clients[userIndex].socket.write(JSON.stringify(post));
        }
    }
    else {
        post.chatid = data.chatid;
        SendDataToMultipleUsers(chats[data.chatid], post);
    }
}

function CreateChat(chatParticipants, socket) {
    chatParticipants.push(socket.name);
    var chatid = chats.push(chatParticipants) - 1;
    
    post.connection = "0022";
    post.chatid = chatid;
    post.isprivate = false;
    post.userlist = chatParticipants;
    post.msg = "";
    socket.write(JSON.stringify(post));
}

function ReadDatabase() {
	var stat1 = fs.statSync(__dirname + "/BazaKorisnika.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		return [];
	}

    var TextFromFile = fs.readFileSync(__dirname + "/BazaKorisnika.json", "utf8");
    var users = JSON.parse(TextFromFile);
    return users;
}

function WriteToDatabase(users) {
    users = JSON.stringify(users);
    fs.writeFile(__dirname + "/BazaKorisnika.json", users, function (err) {
        if (err) return console.log(err);
    });
}

function FindUserByEmail(users, email) {
    index = -1;
    users.forEach(function(user, i) {
        if (user.email == email) {
            index = i;
            return false;
        }
    });
    return index;
}

function AddUserToDataBase(email, password, socket) {

    var users = ReadDatabase();
    var index = FindUserByEmail(users, email)
    if(index !== -1) {
        var post = new Object();
        post.connection = "0002";

        socket.write(JSON.stringify(post));
    } else {
        var friends = [];
        var user = new Object();
        user.email = email;
        user.password = passwordHash.generate(password);
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

    if(index === -1) {
        post.connection = "0005";
        socket.write(JSON.stringify(post));
        return;
    } else {
        if(passwordHash.verify(password, users[index].password) === false) {
            post.connection  = "0006";

            socket.write(JSON.stringify(post));
            return;
        } else {
            post.connection  = "0007";
            socket.name = email;

            var object = new Object;
            object.email = socket.name;
            object.socket = socket;
            object.custom_status = "Online";
			object.current_game = "";
			object.ingame_start_time = new Date();
            object.index = index;
            object.port = port;

            clients.push(object);
            console.log("LOGIN: " + socket.name);
            socket.write(JSON.stringify(post));
            NotifyFriendsAboutChange(email);
            return;
        }
    }
}

function AddLeadingZeros(num, size) {
    var s = num + "";
    while (s.length < size)
		s = "0" + s;
    return s;
}

function CheckForUpdateGamesList(usr_size, usr_datetime, socket) {
    var filename = 'gameslist.dat';
	var usr_last_modified = new Date(usr_datetime);
    var stat1 = fs.statSync(filename);
    var post = new Object();
    if (stat1.mtime.getTime() > usr_last_modified.getTime() || stat1.size !== usr_size) {
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

function ChangeGameActivity(email, current_game) {		//counting user's time spent on some game
	var index = FindUserByEmail(clients, email);
	if (current_game === "") {		//if user stopped playing current game (so currently (s)he isn't playing anything)
		var now = new Date();	//current time (time when user ends playing game which (s)he played till now)
		var difference = now.getTime() - clients[index].ingame_start_time.getTime();	//in milliseconds
		var seconds_elapsed = parseInt (difference/1000);	//there is no need to store milliseconds (and after stringifying numbers in double format take more size than integers)
		console.log( email + " spent " + seconds_elapsed + " seconds playing " + GameNameOnly(CurrUserGame(email)) );
		
		UpdateGameStatistics(email, GameNameOnly(CurrUserGame(email)), seconds_elapsed);
	}
	else {		//if user now started playing game
		clients[index].ingame_start_time = new Date();
		console.log(email + " started playing " + GameNameOnly(current_game) + " on " + clients[index].ingame_start_time);
	}
}

function GameNameOnly(game_with_IP) {
	var IP_position = game_with_IP.lastIndexOf("(");	//precisely, it is position where is opened rounded bracket (which stands before IP address of game server)
	if (IP_position !== -1)
		game_with_IP = game_with_IP.slice(0, IP_position-1);
	return game_with_IP;	//which is now without IP address (if there wasn't IP address, then is original string returned)
}

function NotifyFriendsAboutChange(email) {
    var index = FindUserByEmail(clients, email);
    post = new Object();
    post.connection = "0029";
    post.email = email;
    post.custom_status = CurrUserStatus(email);
    post.current_game = CurrUserGame(email);
    var users = ReadDatabase();
    users.forEach(function(user) {
        if (user.email === email) {
            SendDataToMultipleUsers(user.friends, post);
            return false;
        }
    });
}

function SendDataToMultipleUsers(recipients, data) {    // recipients should be array of strings (usernames)
    var message = JSON.stringify(data);
    recipients.forEach(function(user) {
        var userIndex = FindUserByEmail(clients, user);
        if (userIndex !== -1) {
            clients[userIndex].socket.write(message);
        }
    });
}

function StatusChanged(email, custom_status, current_game) {
	var index = FindUserByEmail(clients, email);
	if (GameNameOnly(clients[index].current_game) !== GameNameOnly(current_game))	//if user stopped playing game which (s)he played before
		ChangeGameActivity(email, current_game);
	clients[index].custom_status = custom_status;
    clients[index].current_game = current_game;
    NotifyFriendsAboutChange(email);
}

function CurrUserStatus(email) {
    var currentStatus = "Offline";
    clients.forEach(function(client) {
        if (client.email === email) {
            currentStatus = client.custom_status;
            return false;
        }
    });
    return currentStatus;
}

function CurrUserGame(email) {
    var currentGame = "";		//if user isn't playing anything, then return empty string
    clients.forEach(function(client) {
        if (client.email === email) {
            currentGame = client.current_game;
            return false;
        }
    });
	return currentGame;
}

function RequestForGameStatistics (email, socket) {
	console.log(socket.name + " sent request for game statistics of user " + email);
	post = new Object();
    post.connection = "0028";
    post.email = email;
	post.stats = ReadGameStatistics(email);
    var index = FindUserByEmail(clients,email);
	if (CurrUserGame(email) !== "") {
        var playsFirstTime = true;
        post.stats.forEach(function(stat) {
			if (stat.game === GameNameOnly(CurrUserGame(email))) {
                stat.time_played += parseInt( ( new Date() - clients[index].ingame_start_time.getTime() ) / 1000 );
                playsFirstTime = false;
				return false;
			}
        });
		if (playsFirstTime) {	//if user is currently playing some game that he never played before (which is still not stored in .json file with stats because user still hasn't exited game)
			var stat = {
				game: GameNameOnly(CurrUserGame(email)),
				time_played: parseInt( ( new Date() - clients[index].ingame_start_time.getTime() ) / 1000 )
			};
			post.stats.push(stat);
		}
	}
    console.log(email + " has played following games:");
    if (post.stats.length > 0) {
        post.stats.forEach(function(stat) {
            console.log("\t" + stat.game + " - " + stat.time_played + " seconds");
        });
    }
    else {
		console.log("\tNone yet");
	}
	socket.write(JSON.stringify(post));
}

function ReadGameStatistics(email) {
	var stat1 = fs.statSync(__dirname + "/GameActivity.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		return [];
	}

    var TextFromFile = fs.readFileSync(__dirname + "/GameActivity.json", "utf8");
    var all_users = JSON.parse(TextFromFile);
    var user_stats = [];
    all_users.forEach(function(u) {
        if (u.user === email) {
            user_stats = u.stats;   //returns all games that user with forwarded email played
            return false;
        }
    });
    return user_stats;
}

function UpdateGameStatistics(email, game, time_played) {
	var content;
	var stat1 = fs.statSync(__dirname + "/GameActivity.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		content = [];
	}
	else {
		var TextFromFile = fs.readFileSync(__dirname + "/GameActivity.json", "utf8");																											//mozda nejde Sync???
		content = JSON.parse(TextFromFile);
	}
	var user_exists = false;
    var game_exists = false;
    content.forEach(function(entry) {
		if (entry.user === email) {
            user_exists = true;
            entry.stats.forEach(function(stat) {
				if (stat.game === game) {
					game_exists = true;
					stat.time_played += time_played;
					return false;
				}
            });
			if (!game_exists) {
				var stat = {
					game: game,
					time_played: time_played
				};
				entry.stats.push(stat);
			}
			return false;
		}
    });
	if (!user_exists) {
		var stat = {
			game: game,
			time_played: time_played
		};
		var obj = new Object();
		obj.user = email;
		obj.stats = new Array();
		
		obj.stats.push(stat);
		
		content.push(obj);
	}
    fs.writeFile(__dirname + "/GameActivity.json", JSON.stringify(content), function (err) {
        if (err)	return console.log(err);
    });
}

function AddFriendToFriendList(users, socket, email) {
    var index = FindUserByEmail(users, socket.name);
    var success = true;
    if (users[index].friends.indexOf(email) !== -1) {
        return false;
    }

    users[index].friends.push(email);
    WriteToDatabase(users);
    console.log("Dodan je prijatelj: " + socket.name + "->" + email);
    return true;
}

function AddFriendUser(email, socket) {
    var post = new Object();

    if(socket.name === email) {
        var post = new Object();
        post.connection = "0013";
        socket.write(JSON.stringify(post));
        return;
    }

    var users = ReadDatabase();
    var index = FindUserByEmail(users, email);


    if(index === -1) {
        post.connection = "0005";
        socket.write(JSON.stringify(post))
        return;
    } else {
        if(AddFriendToFriendList(users, socket, email)) {
            post.connection = "0009";
            socket.write(JSON.stringify(post));
        } else {
            post.connection = "0010";
            socket.write(JSON.stringify(post));
        }
    }
}

function SendFriendsStatus(socket) {
    var post = new Object();
    var users = ReadDatabase();
    var index = FindUserByEmail(users, socket.name);

    var allFriends = [];
    var friends = users[index].friends;

    friends.forEach(function(friend) {
        var object = new Object();
        object.custom_status = "Offline";
		object.current_game = "";
        object.email = friend;
        clients.forEach(function(client) {
            if (friend === client.email) {
                object.custom_status = client.custom_status;
				object.current_game = client.current_game;
                return false;
            }
        });
        allFriends.push(object);
    });
    post.connection = "0012";
    post.friends = allFriends;
    socket.write(JSON.stringify(post));
    return;
}

function GetClientByEmail(email) {
    var foundClient = false;
    clients.forEach(function(client) {
        if (client.email === email) {
            foundClient = client;
            return false;
        }
    });
    return foundClient;
}

var server = net.createServer(function(socket) {

    //Preponavanje klijenta
    socket.name = socket.remoteAddress + ":" + socket.remotePort;

    //dodavanje klijenta u listu
    socket.on('data', function(allData) {
        allData = allData.toString();
        var ranges = [];
        var packetSize = allData.length;
        var numOfConsecutiveEscapeChars = 0;
        var curlyBracketsState = 0;
        var rangeSize = 0;
        for (var i=0; i<packetSize; i++) {
            var currChar = allData[i];
            if (currChar === '\\') {
                numOfConsecutiveEscapeChars++;
            }
            else {
                if (numOfConsecutiveEscapeChars % 2 === 0) {
                    if (currChar === '{') {
                        curlyBracketsState++;
                    }
                    else if (currChar === '}') {
                        if (--curlyBracketsState === 0) {
                            ranges.push(i+1);
                            rangeSize++;
                        }
                    }
                }
                numOfConsecutiveEscapeChars = 0;
            }
        }
        var lowerLimit = 0;
        for (var i=0; i<rangeSize; i++) {
            var upperLimit = ranges[i];

            data = JSON.parse(allData.substring(lowerLimit, upperLimit));

            switch (data.connection) {
                case "0001":
                    AddUserToDataBase(data.email, data.password, socket);
                    break;
                case "0004":
                    LoginUser(data.email, data.password, data.port, socket);
                    break;
                case "0008":
                    AddFriendUser(data.email, socket);
                    break;
                case "0011":
                    SendFriendsStatus(socket);
                    break;
                case "0018":
                    CheckForUpdateGamesList(data.size, data.datetime, socket);
                    break;
                case "0021":
                    CreateChat(data.userlist, socket);
                    break;
                case "0023":
                    SendMsg(data, socket);
                    break;
                case "0024":
                    AddPersonsToChat(data, socket);
                    break;
                case "0025":
                    PersonQuitChat(data, socket);
                    break;
                case "0026":
                    StatusChanged(socket.name, data.custom_status, data.current_game);
                    break;
                case "0027":
                    RequestForGameStatistics (data.email, socket);
                    break;
            }
            lowerLimit = upperLimit;
        }
    });


    socket.on('close', function() {
        var email = socket.name;
        console.log(email + " left.\n");
        for(var i = 0; i < clients.length; i++) {
            if(clients[i]["email"] === email) {
				if (CurrUserGame(email) !== "")		//if user exited application while (s)he was still in game
					ChangeGameActivity(email,"");	//set that (s)he stopped playing game (because we can't track further game activity if (s)he is offline)
                clients.splice(i, 1);
                break;
            }
        }
        NotifyFriendsAboutChange(email);
        NotifyChatRoomsAboutUserLogout(email);
    });

    socket.on('error', function(err) {
        console.log("Error");
    });

});
var serverAddress = ip.address();
var serverPort = 1337;
server.listen(serverPort, serverAddress);
console.log("Server is now listening for requests on the following address and port: " + serverAddress + ":" + serverPort);

fs.appendFileSync(__dirname + "/GameActivity.json","");	//creates file in which will be stored game statistics (if it does not exists)
fs.appendFileSync(__dirname + "/BazaKorisnika.json","");	//creates file in which will be stored list of users and their basic info (if it does not exists)
fs.appendFileSync(__dirname + "/gameslist.dat","");		//creates file in which will be stored game info which will be sent to users (if it does not exists)
