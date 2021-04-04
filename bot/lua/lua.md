# Lua
```Lua is a powerful, efficient, lightweight, embeddable scripting language. It supports procedural programming, object-oriented programming, functional programming, data-driven programming, and data description.```
# engine : class
```Engine provides data access to all controll function and incoming data.```
# engine.message : class
## engine.message.user : string
```The user who sent the message.```
## engine.message.channel : string
```The channel from which the message came.```
## engine.message.text : string
```Incoming message text.```
## engine.message.lang : string
```Estimated language of the incoming message.```
## engine.message.timestamp : number
```Timestamp of incoming message.```
## engine.message.valid : boolean
```Flag that the message is valid```

# engine.chat : class
```A class that allows you to manage chat.```
## engine.chat:send(text) : function
```Send text to the channel from wich the message came.```
## engine.chat.history : class
``` Chat message history.```
### engine.chat.history.getUserMessages(user) : function
```Get user messages from history.```
### engine.chat.history.getMessages(user) : function
```Get all messages from history.```