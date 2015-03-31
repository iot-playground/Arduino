print(wifi.sta.getip())
wifi.setmode(wifi.STATION)
wifi.sta.config("AP","password")
tmr.alarm(1, 8000, 0, function() print(wifi.sta.getip()) dofile('lua_easyiot_doorwindow.lua') end )
