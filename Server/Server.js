//Ucitavanje TCP biblioteke
var ip = require("ip");
var net = require("net");
var fs = require("fs");
var passwordHash = require('password-hash');

var clients = [];
var chats = [];

function notifyChatRoomsAboutUserLogout(email) {
    chats.forEach(function(chatParticipants, index) {
        var userPositionInChat = chatParticipants.indexOf(email);
        if (userPositionInChat !== -1) {
            chatParticipants.splice(userPositionInChat, 1);
            
            var post = new Object();
            post.connection = "0022";
            post.chatid = index.toString();
            post.userlist = chatParticipants;
            post.msg = email + " has left... (went offline)";
        
            sendDataToMultipleUsers(chatParticipants, post);
        }
    });
}

function personQuitsChat(data, socket) {
    var chatGroupToModify = chats[data.chatid];
    chatGroupToModify.splice(chatGroupToModify.indexOf(socket.name), 1);
    
    var post = new Object();
    post.connection = "0022";
    post.chatid = data.chatid;
    post.userlist = chatGroupToModify;
    post.msg = socket.name + " has left...";

    sendDataToMultipleUsers(chatGroupToModify, post);
}

function addPersonsToChat(data, socket) {
    var chatGroupToModify = chats[data.chatid];

    data.userlist.forEach(function(username) {
        var user = getClientByEmail(username);
        if (user) {
            chatGroupToModify.push(username);
        }
    });
    
    data.msg = socket.name + " has added " + data.userlist.join(', ') + " to chat...";
    
    var post = new Object();
    post.connection = "0022";
    post.chatid = data.chatid;
    post.userlist = chatGroupToModify;
    post.msg = data.msg;

    sendDataToMultipleUsers(chatGroupToModify, post);
}

function sendMessage(data, socket) {
    var post = new Object();
    post.connection = "0023";
    post.isprivate = data.isprivate;
    post.msg = socket.name + ": " + data.msg;
    if (data.isprivate) {
        post.chatid = socket.name;
        var client = getClientByEmail(data.chatid);
        if (client) {
            client.socket.write(JSON.stringify(post));
        }
        else {
            post.msg = "You can't send a message to an offline user.";
        }
        post.chatid = data.chatid;
        client = getClientByEmail(socket.name);
        if (client) {
            client.socket.write(JSON.stringify(post));
        }
    }
    else {
        post.chatid = data.chatid;
        sendDataToMultipleUsers(chats[data.chatid], post);
    }
}

function createChat(chatParticipants, socket) {
    chatParticipants.push(socket.name);
    var chatid = chats.push(chatParticipants) - 1;

    var post = new Object();
    post.connection = "0022";
    post.chatid = chatid.toString();
    post.isprivate = false;
    post.userlist = chatParticipants;
    post.msg = socket.name + " has created chat group.";
    sendDataToMultipleUsers(chatParticipants, post);
}

function readDatabase() {
	var stat1 = fs.statSync(__dirname + "/BazaKorisnika.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		return [];
	}

    var TextFromFile = fs.readFileSync(__dirname + "/BazaKorisnika.json", "utf8");
    var users = JSON.parse(TextFromFile);
    return users;
}

function writeToDatabase(users) {
    users = JSON.stringify(users);
    fs.writeFile(__dirname + "/BazaKorisnika.json", users, function (err) {
        if (err) return console.log(err);
    });
}

function getUserByEmail(users, email) {
    var foundUser = false;
    users.forEach(function(user, i) {
        if (user.email == email) {
            foundUser = user;
            return false;
        }
    });
    return foundUser;
}

function addUserToDataBase(email, password, socket) {

    var users = readDatabase();
    var post = new Object();
    if (getUserByEmail(users, email)) {
        post.connection = "0002";
        socket.write(JSON.stringify(post));
    } else {
        var user = new Object();
        user.email = email;
        user.password = passwordHash.generate(password);
        user.friends = [];
        user.incoming_requests = [];
        users.push(user);

        writeToDatabase(users);
        console.log("Following user has been stored: " + user.email + " " + user.password);

        post.connection = "0003";
        socket.write(JSON.stringify(post));
    }
}

function loginUser(email, password, socket) {

    var users = readDatabase();
    var user = getUserByEmail(users, email);
    var post = new Object();

    if(!user) {
        post.connection = "0005";
        socket.write(JSON.stringify(post));
    } else {
        if(passwordHash.verify(password, user.password) === false) {
            post.connection  = "0006";

            socket.write(JSON.stringify(post));
        } else {
            post.connection  = "0007";
            socket.name = email;

            var object = new Object;
            object.email = socket.name;
            object.socket = socket;
            object.custom_status = "Online";
			object.current_game = "";
			object.ingame_start_time = new Date();

            clients.push(object);
            console.log("LOGIN: " + socket.name);
            socket.write(JSON.stringify(post));
            notifyFriendsAboutChange(email);
        }
    }
}

function addLeadingZeros(num, size) {
    var s = num + "";
    while (s.length < size)
		s = "0" + s;
    return s;
}

function checkForGamesListUpdate(userFileSize, userFileDateTicks, socket) {
    var filename = 'gameslist.dat';
	var userFileDate = new Date(userFileDateTicks);new Date()
    var stat1 = fs.statSync(filename);
    var post = new Object();
    if (stat1.mtime.getTime() > userFileDate.getTime() || stat1.size !== userFileSize) {
        post.connection = "0020";
        post.size = addLeadingZeros(stat1.size, 8);		//defining post.size as fixed size (otherwise would number 0 (represented as integer) be converted to string "0" and 1000 to "1000" after stringifying, so packets' size and structure would not be always same!) - can contain value from 0 to 99 999 999
        var fileContent = fs.readFileSync(filename, "binary");
        post.file = fileContent;
        console.log("Games list is not same like latest version. File is sent to client.");
    } else {
        post.connection = "0019";
        console.log("Games list is up to date.");
    }
    socket.write(JSON.stringify(post));
	return;
}

function changeGameActivity(email, currentGameStatus) {		//counting user's time spent on some game
    var client = getClientByEmail(email);
    if (client) {
        if (currentGameStatus === "") {		//if user stopped playing current game (so currently (s)he isn't playing anything)
            var now = new Date();	//current time (time when user ends playing game which (s)he played till now)
            var difference = now.getTime() - client.ingame_start_time.getTime();	//in milliseconds
            var secondsElapsed = parseInt (difference/1000);	//there is no need to store milliseconds (and after stringifying numbers in double format take more size than integers)
            console.log( email + " spent " + secondsElapsed + " seconds playing " + extractGameNameOnly(getCurrUserGameStatus(email)) );
            
            updateGameStatistics(email, extractGameNameOnly(getCurrUserGameStatus(email)), secondsElapsed);
        }
        else {		//if user now started playing game
            client.ingame_start_time = new Date();
            console.log(email + " started playing " + extractGameNameOnly(currentGameStatus) + " on " + client.ingame_start_time);
        }
    }
}

function extractGameNameOnly(gameNameWithIpAndPort) {
    return gameNameWithIpAndPort.replace(/ \(((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])\)$/, '');
}

function notifyFriendsAboutChange(email) {
    var post = new Object();
    post.connection = "0016";
    post.email = email;
    post.custom_status = getCurrUserStatus(email);
    post.current_game = getCurrUserGameStatus(email);
    var users = readDatabase();
    users.forEach(function(user) {
        if (user.email === email) {
            sendDataToMultipleUsers(user.friends, post);
            return false;
        }
    });
}

function sendDataToMultipleUsers(recipients, data) {    // recipients should be array of strings (usernames)
    var message = JSON.stringify(data);
    recipients.forEach(function(user) {
        var client = getClientByEmail(user);
        if (client) {
            client.socket.write(message);
        }
    });
}

function handleStatusChange(email, customStatus, currentGameStatus) {
    var client = getClientByEmail(email);
    if (client) {
        if (extractGameNameOnly(client.current_game) !== extractGameNameOnly(currentGameStatus))	//if user stopped playing game which (s)he played before
    		changeGameActivity(email, currentGameStatus);
        client.custom_status = customStatus;
        client.current_game = currentGameStatus;
        notifyFriendsAboutChange(email);
    }
}

function getCurrUserStatus(email) {
    var currentStatus = "Offline";
    clients.forEach(function(client) {
        if (client.email === email) {
            currentStatus = client.custom_status;
            return false;
        }
    });
    return currentStatus;
}

function getCurrUserGameStatus(email) {
    var currentGame = "";		//if user isn't playing anything, then return empty string
    clients.forEach(function(client) {
        if (client.email === email) {
            currentGame = client.current_game;
            return false;
        }
    });
	return currentGame;
}

function requestForGameStatistics(email, socket) {
	console.log(socket.name + " sent request for game statistics of user " + email);
	var post = new Object();
    post.connection = "0028";
    post.email = email;
	post.stats = readGameStatistics(email);
    var client = getClientByEmail(email);
	if (getCurrUserGameStatus(email) !== "") {
        var playsFirstTime = true;
        post.stats.forEach(function(stat) {
			if (stat.game === extractGameNameOnly(getCurrUserGameStatus(email))) {
                stat.time_played += parseInt( ( new Date() - client.ingame_start_time.getTime() ) / 1000 );
                playsFirstTime = false;
				return false;
			}
        });
		if (playsFirstTime) {	//if user is currently playing some game that he has never played before (which is still not stored in .json file with stats because user still hasn't exited game)
			var stat = {
				game: extractGameNameOnly(getCurrUserGameStatus(email)),
				time_played: parseInt( ( new Date() - client.ingame_start_time.getTime() ) / 1000 )
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

function readGameStatistics(email) {
	var stat1 = fs.statSync(__dirname + "/GameActivity.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		return [];
	}

    var textFromFile = fs.readFileSync(__dirname + "/GameActivity.json", "utf8");
    var users = JSON.parse(textFromFile);
    var userStats = [];
    users.forEach(function(u) {
        if (u.user === email) {
            userStats = u.stats;   //returns all games that user with forwarded email played
            return false;
        }
    });
    return userStats;
}

function updateGameStatistics(email, game, timePlayed) {
	var content;
	var stat1 = fs.statSync(__dirname + "/GameActivity.json");
	if (stat1.size === 0) {	//in case that file is totally empty (it doesn't even contain '[]')
		content = [];
	}
	else {
		var textFromFile = fs.readFileSync(__dirname + "/GameActivity.json", "utf8");
		content = JSON.parse(textFromFile);
	}
	var userExists = false;
    var gameExists = false;
    content.forEach(function(entry) {
		if (entry.user === email) {
            userExists = true;
            entry.stats.forEach(function(stat) {
				if (stat.game === game) {
					gameExists = true;
					stat.time_played += timePlayed;
					return false;
				}
            });
			if (!gameExists) {
				var stat = {
					game: game,
					time_played: timePlayed
				};
				entry.stats.push(stat);
			}
			return false;
		}
    });
	if (!userExists) {
		var stat = {
			game: game,
			time_played: timePlayed
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

function addFriendUser(email, socket) {
    var post = new Object();

    if (socket.name === email) {
        post.connection = "0013";
        socket.write(JSON.stringify(post));
        return;
    }

    var users = readDatabase();
    var user = getUserByEmail(users, email);


    if (!user) {
        post.connection = "0005";
    } else {
        if (user.incoming_requests.indexOf(socket.name) !== -1) {    // friend request was previously sent by user and still hasn't been processed by another user
            post.connection = "0014";
        } else if (user.friends.indexOf(socket.name) !== -1) {    // users are already friends
            post.connection = "0010";
        } else {  // friend request wasn't previously sent or it was already rejected by another user
            var subject = getUserByEmail(users, socket.name);
            if (subject.incoming_requests.indexOf(email) !== -1) {
                var incomingRequests = subject.incoming_requests;
                incomingRequests.splice(incomingRequests.indexOf(email), 1);
                establishFriendship(socket, subject, user);
                writeToDatabase(users);
                return;
            } else {
                user.incoming_requests.push(socket.name);
                post.connection = "0009";
                sendUserFriendRequest(email, socket.name);
                writeToDatabase(users);
                console.log("User " + socket.name + " has sent friend request to user " + email);
            }
        }
    }
    socket.write(JSON.stringify(post));
}

function sendUserFriendRequest(destination, interestedUser) {
    var client = getClientByEmail(destination);
    if (client) {
        var post = new Object();
        post.connection = "0017";
        post.email = interestedUser;
        client.socket.write(JSON.stringify(post));
    }
}

function establishFriendship(socket, subject, object) {
    object.friends.push(subject.email);
    subject.friends.push(object.email);
    sendInformationAboutNewlyAddedFriend(socket, object.email);
    var client = getClientByEmail(object.email);
    if (client) {
        sendInformationAboutNewlyAddedFriend(client.socket, subject.email);
    }
    console.log("Users " + subject.email + " and " + object.email + " have now become friends");
}

function sendInformationAboutNewlyAddedFriend(socket, user) {
    var post = new Object();
    post.connection = "0016";
    post.email = user;
    post.custom_status = getCurrUserStatus(user);
    post.current_game = getCurrUserGameStatus(user);
    socket.write(JSON.stringify(post));
}

function handleFriendRequest(data, socket) {
    var users = readDatabase();
    var subject = getUserByEmail(users, socket.name);
    var incomingRequests = subject.incoming_requests;
    incomingRequests.splice(incomingRequests.indexOf(data.email), 1);
    if (data.accept) {
        establishFriendship(socket, subject, getUserByEmail(users, data.email));
    }
    writeToDatabase(users);
}

function sendFriendsList(socket) {
    var post = new Object();
    var users = readDatabase();
    var user = getUserByEmail(users, socket.name);

    var allFriends = [];
    var friends = user.friends;

    friends.forEach(function(friend) {
        var object = new Object();
        object.email = friend;
        object.custom_status = getCurrUserStatus(friend);
		object.current_game = getCurrUserGameStatus(friend);
        allFriends.push(object);
    });
    post.connection = "0012";
    post.friends = allFriends;
    post.friend_requests = user.incoming_requests;
    socket.write(JSON.stringify(post));
    return;
}

function getClientByEmail(email) {
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
                    addUserToDataBase(data.email, data.password, socket);
                    break;
                case "0004":
                    loginUser(data.email, data.password, socket);
                    break;
                case "0008":
                    addFriendUser(data.email, socket);
                    break;
                case "0011":
                    sendFriendsList(socket);
                    break;
                case "0015":
                    handleFriendRequest(data, socket);
                    break;
                case "0018":
                    checkForGamesListUpdate(data.size, data.datetime, socket);
                    break;
                case "0021":
                    createChat(data.userlist, socket);
                    break;
                case "0023":
                    sendMessage(data, socket);
                    break;
                case "0024":
                    addPersonsToChat(data, socket);
                    break;
                case "0025":
                    personQuitsChat(data, socket);
                    break;
                case "0026":
                    handleStatusChange(socket.name, data.custom_status, data.current_game);
                    break;
                case "0027":
                    requestForGameStatistics (data.email, socket);
                    break;
            }
            lowerLimit = upperLimit;
        }
    });


    socket.on('close', function() {
        var email = socket.name;
        console.log(email + " left.");
        for(var i = 0; i < clients.length; i++) {
            if(clients[i]["email"] === email) {
				if (getCurrUserGameStatus(email) !== "")		//if user exited application while (s)he was still in game
					changeGameActivity(email,"");	//set that (s)he stopped playing game (because we can't track further game activity if (s)he is offline)
                clients.splice(i, 1);
                break;
            }
        }
        notifyFriendsAboutChange(email);
        notifyChatRoomsAboutUserLogout(email);
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
