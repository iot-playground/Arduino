-- see http://iot-playgroud.com for details

easyiot_username = "admin"
easyiot_password = "test"
easyiot_server = "192.168.1.5"
easyiot_node_address = "N9S0"

-- character table string
local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'


delay = 0
gpio.mode(4,gpio.INT,gpio.PULLUP)

function sendStatus(status)

command = 'ControlOff'
if (status == 1) then
	command = 'ControlOn'
end

conn = nil
conn=net.createConnection(net.TCP, 0) 

-- send status to sensor node

conn:on("receive", function(conn, payload) 
                       success = true
                       print(payload) 
                       end) 

-- when connected, request page (send parameters to a script)
conn:on("connection", function(conn, payload) 
                       print('\nConnected') 
                       conn:send("POST /Api/EasyIoT/Control/Module/Virtual/"..easyiot_node_address.."/"..command.." HTTP/1.1\r\n" 
                        .."Host: "..easyiot_server.."\r\n" 
						.."Content-Length: 0\r\n" 
               		   .."Connection: keep-alive\r\n"
					   .."Authorization: Basic "..credentals.."\r\n"
                        .."Accept: */*\r\n" 
                        .."User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n" 
                        .."\r\n")
                       end) 
-- when disconnected, let it be known
conn:on("disconnection", function(conn, payload) print('\nSend status') end)
                                             
conn:connect(80,easyiot_server) 
end



function changestatus(level)
   x = tmr.now()
   if x > delay then
      delay = tmr.now()+250000
	  sendStatus(level)	  	 
	  print(level)	 	  
    end
 end
 
 
-- encoding basic authorization
function enc(data)
    return ((data:gsub('.', function(x) 
        local r,b='',x:byte()
        for i=8,1,-1 do r=r..(b%2^i-b%2^(i-1)>0 and '1' or '0') end
        return r;
    end)..'0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
        if (#x < 6) then return '' end
        local c=0
        for i=1,6 do c=c+(x:sub(i,i)=='1' and 2^(6-i) or 0) end
        return b:sub(c+1,c+1)
    end)..({ '', '==', '=' })[#data%3+1])
end 
 
 
credentals = enc(easyiot_username..":"..easyiot_password)
 
level1 = gpio.read(4)
sendStatus(level1)

gpio.trig(4, "both",changestatus)





