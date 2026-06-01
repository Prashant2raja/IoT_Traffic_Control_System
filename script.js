//////////////////////////////////////////////////////////
// FIREBASE CONFIG
//////////////////////////////////////////////////////////

const firebaseConfig = {
  apiKey: "AIzaSyA2dmUCogmK_XpAGRpGzIk0WAMLOdi3-mQ",
  authDomain: "traffic-control-26.firebaseapp.com",
  databaseURL: "https://traffic-control-26-default-rtdb.firebaseio.com",
  projectId: "traffic-control-26",
  storageBucket: "traffic-control-26.firebasestorage.app",
  messagingSenderId: "238459371020",
  appId: "1:238459371020:web:20fed4c71473a69626b829"
};

firebase.initializeApp(firebaseConfig);

const db = firebase.database();

//////////////////////////////////////////////////////////
// REALTIME DATABASE REFERENCES
//////////////////////////////////////////////////////////

const northRef =
db.ref("TrafficSystem/density/north");

const southRef =
db.ref("TrafficSystem/density/south");

const eastRef =
db.ref("TrafficSystem/density/east");

const westRef =
db.ref("TrafficSystem/density/west");

const statusRef =
db.ref("TrafficSystem/status");

const modeRef =
db.ref("TrafficSystem/currentMode");

const timerRef =
db.ref("TrafficSystem/timer");

//////////////////////////////////////////////////////////
// UPDATE DENSITY
//////////////////////////////////////////////////////////

northRef.on("value",(snap)=>{
    document.getElementById("north").innerHTML =
    snap.val();
});

southRef.on("value",(snap)=>{
    document.getElementById("south").innerHTML =
    snap.val();
});

eastRef.on("value",(snap)=>{
    document.getElementById("east").innerHTML =
    snap.val();
});

westRef.on("value",(snap)=>{
    document.getElementById("west").innerHTML =
    snap.val();
});

//////////////////////////////////////////////////////////
// SIGNAL STATUS
//////////////////////////////////////////////////////////

statusRef.on("value",(snap)=>{

    let status = snap.val();

    console.log("STATUS:", status);

    document.getElementById("statusText")
    .innerHTML = status;

    resetLights();

    ////////////////////////////////////////////////////
    // NS GREEN
    ////////////////////////////////////////////////////

    if(status === "NS_GREEN_ACTIVE")
    {
        document.getElementById("nsGreen")
        .classList.add("active");

        document.getElementById("ewRed")
        .classList.add("active");
    }

    ////////////////////////////////////////////////////
    // EW GREEN
    ////////////////////////////////////////////////////

    else if(status === "EW_GREEN_ACTIVE")
    {
        document.getElementById("ewGreen")
        .classList.add("active");

        document.getElementById("nsRed")
        .classList.add("active");
    }
	else if(status === "ALL_RED")
{
    document.getElementById("nsRed")
    .classList.add("active");

    document.getElementById("ewRed")
    .classList.add("active");
}

    ////////////////////////////////////////////////////
    // YELLOW
    ////////////////////////////////////////////////////

    else if(status === "YELLOW")
{
    resetLights();

    document.getElementById("nsYellow")
    .classList.add("active");

    document.getElementById("ewYellow")
    .classList.add("active");

    document.getElementById("statusText")
    .innerHTML = "YELLOW BLINKING";
}

});

//////////////////////////////////////////////////////////
// MODE STATUS
//////////////////////////////////////////////////////////

modeRef.on("value",(snap)=>{

    let mode = snap.val();

    document.getElementById("modeStatus")
    .innerHTML = mode;
});


//////////////////////////////////////////////////////////
// TIMER
//////////////////////////////////////////////////////////

timerRef.on("value",(snap)=>{

    let time = snap.val();

    if(time == null)
        return;

    document.getElementById("countdown")
    .innerHTML = time;

    //////////////////////////////////////////////////////
    // COLOR CHANGE
    //////////////////////////////////////////////////////

    if(time <= 5)
    {
        document.getElementById("countdown")
        .style.color = "#ff0000";
    }

    else if(time <= 10)
    {
        document.getElementById("countdown")
        .style.color = "#ffff00";
    }

    else
    {
        document.getElementById("countdown")
        .style.color = "#38bdf8";
    }
});

//////////////////////////////////////////////////////////
// RESET LIGHTS
//////////////////////////////////////////////////////////

function resetLights()
{
    let lights = document.querySelectorAll(".light");

    lights.forEach(light=>{
        light.classList.remove("active");
    });
}

//////////////////////////////////////////////////////////
// AUTO MODE
//////////////////////////////////////////////////////////

function setAuto()
{
    db.ref("TrafficSystem/mode")
    .set("AUTO");
}

//////////////////////////////////////////////////////////
// MANUAL MODE
//////////////////////////////////////////////////////////

function setManual()
{
    db.ref("TrafficSystem/mode")
    .set("MANUAL");
}

//////////////////////////////////////////////////////////
// SEND COMMAND
//////////////////////////////////////////////////////////

function sendCommand(cmd)
{
    db.ref("TrafficSystem/command")
    .set(cmd);
}
