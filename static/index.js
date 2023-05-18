const baseUri = "https://bc731a7c-153e-4011-899a-fee8ff2e1854.mock.pstmn.io";

const selectElement = document.getElementById("ssid-options");
const ssidElement = document.getElementById("ssid");
const passwordElement = document.getElementById("password");
const deviceStatusElement = document.getElementById("status");
const deviceIdElement = document.getElementById("device-id");
const loadingElement = document.getElementById("loading-container");

function openSelectOptions() {
  cleanOptionsFilter();
  selectElement.classList.add("options-opened");
}

function closeSelectOptions() {
  setTimeout(() => {
    selectElement.classList.remove("options-opened");
  }, 200);
}

function selectSsid(value) {
  ssidElement.value = value;
}

function filterOptions() {
  const value = ssidElement.value.toLowerCase();
  const options = selectElement.childNodes;
  options.forEach(option => {
    option.style.display = option.id.toLowerCase().indexOf(value) > -1 ? '' : 'none';
  });
}

function cleanOptionsFilter() {
  const options = selectElement.childNodes;
  options.forEach(option => {
    if (option) {
      if (option.style) {
        option.style.display = '';
      }
    }
  });
}

async function getDeviceInfo() {
  const endpoint = "/network/info";
  const url = baseUri + endpoint;

  const result = await fetch(url);
  const data = await result.json();

  return data;
}

async function getAvailableNetworks() {
  const endpoint = "/network/scan";
  const url = baseUri + endpoint;

  let result = await fetch(url);

  while (result.status === 202) {
    result = await new Promise((resolve) => {
      setTimeout(() => {
        resolve(fetch(url));
      }, 5000);
    });
  }

  const data = await result.json();
  return data;
}

async function setNetworkConfig() {
  const endpoint = "/network/configure";
  const url = baseUri + endpoint;

  const data = {
    ssid: ssidElement.value,
    password: passwordElement.value,
  };

  const result = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });

  if (result.status === 201) {
    loadingElement.style.display = "flex";
    setTimeout(() => {
      location.reload();
    }, 5000);
    return;
  }

  alert("Desculpe, não foi possível configurar a rede!");
}

async function onload() {
  const { id, ipv4, ssid, status } = await getDeviceInfo();

  deviceIdElement.innerHTML = id;
  ssidElement.value = ssid;
  deviceStatusElement.innerHTML = status ? "conectado" : "desconectado";

  getAvailableNetworks().then((networks) => {
    if (!networks.length) {
      return;
    }

    networks = networks.sort((a, b) => b.signal - a.signal);

    selectElement.innerHTML = "";

    networks.forEach(({ ssid, signal, secure }) => {
      const option = document.createElement("div");
      option.classList.add("font-medium");
      option.id = ssid;
      option.innerHTML = `${ssid} (${signal}%)${secure ? " *" : ""}`;
      option.onclick = () => selectSsid(ssid);
      selectElement.appendChild(option);
    });
  });

  loadingElement.style.display = "none";
}

onload();
