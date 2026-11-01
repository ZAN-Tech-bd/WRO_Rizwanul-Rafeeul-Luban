@echo off
rem ============================================================
rem  Starts the EasyEDA WebSocket bridge server (local, port 49620)
rem  Run this AFTER every PC reboot, then in EasyEDA Pro:
rem  API Gateway -> Reconnect
rem ============================================================
cd /d D:\projects\easyeda-api-skill

echo Starting EasyEDA bridge server (minimized window)...
start "EasyEDA Bridge" /min cmd /c "npm run server > bridge-server.log 2>&1"

echo Waiting for it to come up...
timeout /t 5 /nobreak >nul

curl -s --max-time 5 http://127.0.0.1:49620/health
echo.
echo.
echo If the JSON above says "service":"easyeda-bridge","status":"ok" - the bridge is running.
echo Now open EasyEDA Pro and click:  API Gateway -^> Reconnect
echo (Expect the toast: Bridge connected, port 49620)
echo.
echo This window can be closed - the bridge keeps running minimized.
pause
