function formatLargestUnit(seconds) {
  const units = [
    { label: "day", value: 86400 },
    { label: "hour", value: 3600 },
    { label: "minute", value: 60 },
    { label: "second", value: 1 },
  ];

  for (const unit of units) {
    if (seconds <= 60) {
      return "< 1 min";
    }
    if (seconds >= unit.value || unit.label === "second") {
      const count = Math.floor(seconds / unit.value);
      return `${count} ${unit.label}${count === 1 ? "" : "s"}`;
    }
  }
}

async function nodesStale() {
	const apiUrl = "/api/nodes"
  const tableBody = document.getElementById("nodes-body");

  try {
    // 1. Fetch data from the API
    const response = await fetch(apiUrl);

    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const nodes = await response.json();
	const any_nodes_stale = false;


    tableBody.innerHTML = "";

    nodes.forEach((node) => {
      const row = document.createElement("tr");
	    if (node.test_node) {
	      return;
      } else if (node.node_stale) {
	      any_nodes_stale = true;
	}
    });
  } catch (error) {
    console.error("Failed to load main status:", error);
  }
}

	
async function renderNodes() {
  const apiUrl = "/api/nodes";
  const tableBody = document.getElementById("nodes-body");

  try {
    // 1. Fetch data from the API
    const response = await fetch(apiUrl);

    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const nodes = await response.json();

    tableBody.innerHTML = "";

    nodes.forEach((node) => {
      const row = document.createElement("tr");
	    if (node.test_node) {
	      return;
      }
	    
      if (node.last_backlog) {
        row.classList.add("w3-yellow");
      } else if (node.node_stale) {
        row.classList.add("w3-red");
      } else {
        row.classList.add("w3-green");
      }

      const rowValues = [
        node.node_id,
        node.hive_name ?? "N/A",
        node.gateway_name ?? "N/A",
        formatLargestUnit(node.reading_age_s),
        node.node_stale
          ? "Stale"
          : node.last_backlog
            ? "Clearing Backlog"
            : "Online",
      ];

      rowValues.forEach((text) => {
        const cell = document.createElement("td");
        cell.textContent = text; // Using textContent prevents XSS attacks
        row.appendChild(cell);
      });

      // Sleep time input
      const sleepCell = document.createElement("td");

      const sleepInput = document.createElement("input");
      sleepInput.type = "number";
      sleepInput.min = "5";
      sleepInput.value = node.expected_interval_s ?? "";
      sleepInput.className = "w3-input";
      sleepInput.id = `sleep-${node.node_id}`;

      sleepCell.appendChild(sleepInput);
      row.appendChild(sleepCell);

      // Save button
      const saveCell = document.createElement("td");

      const saveButton = document.createElement("button");
      saveButton.textContent = "Save";
      saveButton.className = "w3-button w3-blue w3-round";
      saveButton.type = "button";

      saveButton.addEventListener("click", () => {
        saveSleepTime(node.node_id, sleepInput.value);
      });

      saveCell.appendChild(saveButton);
      row.appendChild(saveCell);
      tableBody.appendChild(row);

      const moreRow = [
        `${node.command ?? "N/A"}:${node.value ?? "N/A"}`,
        node.queued,
        node.sent,
      ];

      moreRow.forEach((text) => {
        const cell = document.createElement("td");
        cell.textContent = text; // Using textContent prevents XSS attacks
        row.appendChild(cell);
      });
    });
  } catch (error) {
    console.error("Failed to load table data:", error);
    tableBody.innerHTML = `<tr><td colspan="4">Error loading data.</td></tr>`;
  }
}

async function saveSleepTime(node_id, sleep_time) {
	try {
    const response = await fetch('/api/commands', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
	      node_id: node_id,
        command: "interval",
	      value: sleep_time
      })
    });

    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const result = await response.json();
    console.log('Set sleep time.', result);
  } catch (error) {
    console.error('Error:', error);
  }	
}

async function renderGateways() {
  const apiUrl = "/api/gateways";
  const tableBody = document.getElementById("gateways-body");

  try {
    // 1. Fetch data from the API
    const response = await fetch(apiUrl);

    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    const gateways = await response.json();
    tableBody.innerHTML = "";

    gateways.forEach((gateway) => {
      const row = document.createElement("tr");

      if (gateway.gateway_stale) {
        row.classList.add("w3-red");
      } else {
        row.classList.add("w3-green");
      }

      const rowValues = [
        gateway.gateway_id,
        gateway.gateway_name ?? "N/A",
        formatLargestUnit(gateway.heartbeat_age_s),
        gateway.gateway_stale ? "Stale" : "Online",
      ];

      rowValues.forEach((text) => {
        const cell = document.createElement("td");
        cell.textContent = text; // Using textContent prevents XSS attacks
        row.appendChild(cell);
      });
	    tableBody.appendChild(row);
    });
  } catch (error) {
    console.error("Failed to load table data:", error);
    tableBody.innerHTML = `<tr><td colspan="4">Error loading data.</td></tr>`;
  }
}
// Run when the page loads
document.addEventListener("DOMContentLoaded", renderNodes);
setInterval(renderNodes, 5000);

document.addEventListener("DOMContentLoaded", renderGateways);
setInterval(renderGateways, 5000);
