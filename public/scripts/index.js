const firebaseConfig = {
  apiKey: "AIzaSyB6OcY25hnUQNw4g9E4CVC_i4eQJiLNI7U",
  authDomain: "database-walet.firebaseapp.com",
  databaseURL: "https://database-walet-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "database-walet",
  storageBucket: "database-walet.appspot.com",
  messagingSenderId: "101892265939",
  appId: "1:101892265939:web:1fd7b539c07c768c520126"
};

firebase.initializeApp(firebaseConfig);
const auth = firebase.auth();
const db = firebase.database();

const logoutNavElement = document.querySelector('#logout-link');
const loginNavElement = document.querySelector('#login-link');
const signedOutMessageElement = document.querySelector('#signedOutMessage');
const dashboardElement = document.querySelector("#dashboardSignedIn");
const userDetailsElement = document.querySelector('#user-details');

const setupUI = (user) => {
  if (user) {
      logoutNavElement.style.display = 'block';
      loginNavElement.style.display = 'none';
      signedOutMessageElement.style.display = 'none';
      dashboardElement.style.display = 'block';
      userDetailsElement.style.display = 'none';
      userDetailsElement.innerHTML = user.email;

      var uid = user.uid;
      var dbPathBtn1 = `UsersData/${uid}/outputs/digital/4`;
      var dbPathBtn2 = `UsersData/${uid}/outputs/digital/5`;
      var dbPathSlider1 = `UsersData/${uid}/outputs/threshold`;
      var dbPathshtTemp = `UsersData/${uid}/sensor/temperature`;
      var dbPathshtHum = `UsersData/${uid}/sensor/humidity`;

      var btn1State = document.getElementById('btn1State');
      var dbBtn1 = firebase.database().ref().child(dbPathBtn1);

      dbBtn1.on('value', snap => {
          if (snap.val() == 1) {
              btn1State.innerText = "ON";
              document.getElementById('btn1Indicator').style.backgroundColor = "green";
          } else {
              btn1State.innerText = "OFF";
              document.getElementById('btn1Indicator').style.backgroundColor = "red";
          }
      });

      const btn1On = document.getElementById('btn1On');
      const btn1Off = document.getElementById('btn1Off');

      btn1On.onclick = () => {
          firebase.database().ref().child(dbPathBtn1).set(1);
      };
      btn1Off.onclick = () => {
          firebase.database().ref().child(dbPathBtn1).set(0);
      };

      var btn2State = document.getElementById('btn2State');
      var dbBtn2 = firebase.database().ref().child(dbPathBtn2);

      dbBtn2.on('value', snap => {
          const isButton2On = snap.val() == 1;
          btn2State.innerText = isButton2On ? "ON" : "OFF";
          updateButton1State(isButton2On);
      });

      const btn2On = document.getElementById('btn2On');
      const btn2Off = document.getElementById('btn2Off');

      btn2On.onclick = () => {
          firebase.database().ref().child(dbPathBtn2).set(1);
      };
      btn2Off.onclick = () => {
          firebase.database().ref().child(dbPathBtn2).set(0);
      };

      const updateButton1State = (isButton2On) => {
          btn1On.disabled = isButton2On;
          btn1Off.disabled = isButton2On;
      };

      var dbshtTemp = firebase.database().ref().child(dbPathshtTemp);
      const shtTemp = document.getElementById('shtTemp');

      dbshtTemp.on('value', snap => {
          lastTemp = snap.val().toFixed(2);
          shtTemp.innerText = `${lastTemp} ºC`;
      });

      var dbshtHumi = firebase.database().ref().child(dbPathshtHum);
      const shtHumi = document.getElementById('shtHumi');

      dbshtHumi.on('value', snap => {
          lastHumi = snap.val().toFixed(2);
          shtHumi.innerText = `${lastHumi} %`;
      });

      var sld1Value = document.getElementById('sld1Value');
      var dbSld1 = firebase.database().ref().child(dbPathSlider1);
      const sld1 = document.getElementById('sld1');

      dbSld1.on('value', snap => {
          sld1Value.innerText = `${snap.val()} ºC`;
          sld1.value = snap.val();
      });

      sld1.onchange = () => {
          firebase.database().ref().child(dbPathSlider1).set(Number(sld1.value));
      };

  } else {
      logoutNavElement.style.display = 'none';
      loginNavElement.style.display = 'block';
      signedOutMessageElement.style.display = 'block';
      dashboardElement.style.display = 'none';
      userDetailsElement.style.display = 'none';
  }
};

firebase.auth().onAuthStateChanged(user => {
  setupUI(user);
});
