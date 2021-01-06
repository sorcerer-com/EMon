var loading = true;
$("header").ready(function () {
	if (loading) {
		$("header span").html("(loading...)").css("font-size", "0.8em");
	}
});

$(() => {
	loadData();
	// only consumption page has realtime data, so else refresh once per minute
	if (window.location.href.includes("consumption.html")) {
		setInterval(loadData, 5000);
	} else {
		setInterval(loadData, 60000);
	}
});

// refresh data only if the page is visible
var active = true;
$(window).focus(() => active = true).blur(() => active = false);

function loadData() {
	if (!active)
		return;

	$.get("/data.json", (json) => {
		loading = false;
		$("header").css("background-color", "#1995AD");
		$("header span").html("");
		$(window).trigger("data", json);
	}).fail(() => {
		if (loading) {
			$("header").css("background-color", "#A81919");
			$("header span").html("(the data cannot be loaded, please refresh)");
		}
	});
}

$(window).on("data", function (e, data) {
	// show current time warning
	var date = new Date(data.time);
	date.setMinutes(date.getMinutes() + date.getTimezoneOffset());
	if (Math.abs(new Date() - date) > 60 * 1000) { // if time difference is more than 1 minute
		$("header span").html("<a href='settings.html'>(set current time)</a>");
		$("header span a").css("font-size", "0.8em").css("color", "red");
	}

	// TODO: maybe add tariffs hint - price
	for (var t = 0; t < data.tariffsCount - 1; t++) {
		if (data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1]) {
			// remove unused tariffs
			$(`.tariff-${t + 1}`).remove();
			$(`.ch-year-tariff-${t + 1}`).remove();
		}
	}

	for (var m = 0; m < data.monitorsCount; m++) {
		if (data.settings.monitorsNames[m] != "") {
			// add monitors hints
			$(`.monitor-${m + 1}`).prop("title", data.settings.monitorsNames[m]);
		} 
		else if (!isMonitorVisible(data, m)) {
			// remove monitors without name
			$(`.monitor-${m + 1}`).remove();
			$(`.section-last-24-hours-monitor-${m + 1}`).remove();
			$(`.section-last-month-monitor-${m + 1}`).remove();
			$(`.section-last-year-monitor-${m + 1}`).remove();
		}
	}

	if ($(".swiper-container").length > 0) {
		// mobile
		InitializeSwiper();
	}

	// Consumption
	//// Total
	// total
	var total = 0;
	var totalPrice = 0.0;
	var values = [];
	for (var t = 0; t < data.tariffsCount; t++) {
		values.push(0);
		for (var m = 0; m < data.monitorsCount; m++) {
			values[t] += toKilo(data.current.month[m][t]);
		}
		total += values[t];
		totalPrice += values[t] * data.settings.tariffPrices[t];
	}
	$(".cr-total-price").html(`${totalPrice.toFixed(2)} ${data.settings.currencySymbols}`);
	$(".cr-total-value").html(`(${total.toFixed(2)} kWh)`);
	// by tariffs
	if (total == 0) total = 1;
	for (var t = 0; t < data.tariffsCount; t++) {
		$(`.cr-total.tariff-${t + 1} span`)
			.html(`${values[t].toFixed(2)} kWh (${Math.round(values[t] / total * 100)} %)`);
	}
	// by monitors
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = 0;
		for (var t = 0; t < data.tariffsCount; t++) {
			value += toKilo(data.current.month[m][t]);
		}
		$(`.cr-total.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}
	// prev month
	var prevBillMonth = getBillMonth(data) - 1;
	if (prevBillMonth < 1) prevBillMonth += 12;
	var total = 0;
	var totalPrice = 0.0;
	var values = [];
	for (var t = 0; t < data.tariffsCount; t++) {
		values.push(0);
		for (var m = 0; m < data.monitorsCount; m++) {
			values[t] += toKilo(data.months[m][t][prevBillMonth - 1]);
		}
		total += values[t];
		totalPrice += values[t] * data.settings.tariffPrices[t];
	}
	$(".cr-total-prevMonth-price span")
		.html(`${totalPrice.toFixed(2)} ${data.settings.currencySymbols}`);
	$(".cr-total-prevMonth-value")
		.html(` (${total.toFixed(2)} kWh, ${values.map(function (a) { return a.toFixed(2); }).join(" / ")})`);

	//// Current Usage
	var total = 0;
	for (var m = 0; m < data.monitorsCount; m++) {
		total += data.current.energy[m];
	}
	if (total == 0) total = 1;
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = data.current.energy[m];
		$(`.cr-current-usage.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} W (${Math.round(value / total * 100)} %)`);
	}
	$(`.cr-current-usage.voltage span`).html(`${data.current.voltage.toFixed(2)} V`);

	//// Current Hour
	var total = 0;
	for (var m = 0; m < data.monitorsCount; m++) {
		total += toFloat(data.current.hour[m]);
	}
	if (total == 0) total = 1;
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = toFloat(data.current.hour[m]);
		$(`.cr-current-hour.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} Wh (${Math.round(value / total * 100)} %)`);
	}

	//// Current Day
	// total
	var total = 0;
	for (var m = 0; m < data.monitorsCount; m++) {
		for (var t = 0; t < data.tariffsCount; t++) {
			total += toKilo(data.current.day[m][t]);
		}
	}
	$(`.cr-current-day-total span`)
		.html(`${total.toFixed(2)} kWh`);
	if (total == 0) total = 1;
	// by monitors
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = 0;
		for (var t = 0; t < data.tariffsCount; t++) {
			value += toKilo(data.current.day[m][t]);
		}
		$(`.cr-current-day.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}
	// by tariffs
	for (var t = 0; t < data.tariffsCount; t++) {
		var value = 0;
		for (var m = 0; m < data.monitorsCount; m++) {
			value += toKilo(data.current.day[m][t]);
		}
		$(`.cr-current-day.tariff-${t + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}

	//// Last 24 Hours
	// total
	var total = 0;
	var values = [];
	for (var m = 0; m < data.monitorsCount; m++) {
		values.push(0);
		for (var h = 0; h < 24; h++) {
			values[m] += toKilo(data.hours[m][h]);
		}
		total += values[m];
	}
	$(`.cr-last-24-hours-total span`).html(`${total.toFixed(2)} kWh`);
	$(`.cr-last-24-hours-average span`)
		.html(`${(total / 24).toFixed(2)} kWh (${values.map(function (a) { return (a / 24).toFixed(2); }).join("/")})`);
	// by tariffs
	var values = [];
	for (var t = 0; t < data.tariffsCount; t++) {
		values.push(0);
	}
	for (var h = 0; h < 24; h++) {
		var value = 0;
		for (var m = 0; m < data.monitorsCount; m++) {
			value += toKilo(data.hours[m][h]);
		}

		if (h >= data.settings.tariffStartHours[0] && h < data.settings.tariffStartHours[1])
			values[0] += value;
		else if (h >= data.settings.tariffStartHours[1] && h < data.settings.tariffStartHours[2])
			values[1] += value;
		else if (h >= data.settings.tariffStartHours[2] || h < data.settings.tariffStartHours[0])
			values[2] += value;
	}
	if (total == 0) total = 1;
	for (var t = 0; t < data.tariffsCount; t++) {
		$(`.cr-last-24-hours.tariff-${t + 1} span`)
			.html(`${values[t].toFixed(2)} kWh (${Math.round(values[t] / total * 100)} %)`);
	}
	// by monitors
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = 0;
		for (var h = 0; h < 24; h++) {
			value += toKilo(data.hours[m][h]);
		}
		$(`.cr-last-24-hours.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}
	// current hour
	var total = 0;
	var values = [];
	for (var m = 0; m < data.monitorsCount; m++) {
		values.push(toKilo(data.current.hour[m]))
		total += values[m];
	}
	$(`.cr-last-24-hours-current span`)
		.html(`${total.toFixed(2)} kWh (${values.map(function (a) { return a.toFixed(2); }).join("/")})`);

	//// Last Month
	// total
	var total = 0;
	var values = [];
	var dt = new Date(data.time);
	var daysCount = new Date(dt.getUTCFullYear(), dt.getUTCMonth(), 0).getDate(); // get previous month length
	for (var t = 0; t < data.tariffsCount; t++) {
		values.push(0);
		for (var d = 0; d < daysCount; d++) {
			for (var m = 0; m < data.monitorsCount; m++) {
				values[t] += toKilo(data.days[m][t][d]);
			}
		}
		total += values[t];
	}
	$(`.cr-last-month-total span`).html(`${total.toFixed(2)} kWh`);
	$(`.cr-last-month-average span`)
		.html(`${(total / daysCount).toFixed(2)} kWh (${values.map(function (a) { return (a / daysCount).toFixed(2); }).join("/")})`);
	if (total == 0) total = 1;
	// by tariffs
	for (var t = 0; t < data.tariffsCount; t++) {
		var value = 0;
		for (var d = 0; d < daysCount; d++) {
			for (var m = 0; m < data.monitorsCount; m++) {
				value += toKilo(data.days[m][t][d]);
			}
		}
		$(`.cr-last-month.tariff-${t + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}
	// by monitors
	for (var m = 0; m < data.monitorsCount; m++) {
		var value = 0;
		for (var d = 0; d < daysCount; d++) {
			for (var t = 0; t < data.tariffsCount; t++) {
				value += toKilo(data.days[m][t][d]);
			}
		}
		$(`.cr-last-month.monitor-${m + 1} span`)
			.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
	}
	// current day
	var total = 0;
	var values = [];
	for (var t = 0; t < data.tariffsCount; t++) {
		values.push(0);
		for (var m = 0; m < data.monitorsCount; m++) {
			values[t] += toKilo(data.current.day[m][t]);
		}
		total += values[t];
	}
	$(`.cr-last-month-current span`)
		.html(`${total.toFixed(2)} kWh (${values.map(function (a) { return a.toFixed(2); }).join("/")})`);


	// Monitors
	//// Last 24 hours per monitor
	for (var m = 0; m < data.monitorsCount; m++) {
		if (data.settings.monitorsNames[m] != "") {
			$(`.section-title.last-24-hours-monitor-${m + 1} span`).html(` (${data.settings.monitorsNames[m]})`);
		}

		// total
		var total = 0;
		var values = [];
		for (var t = 0; t < data.tariffsCount; t++) {
			values.push(0);
		}
		for (var h = 0; h < 24; h++) {
			var value = toKilo(data.hours[m][h]);
			total += value;

			if (h >= data.settings.tariffStartHours[0] && h < data.settings.tariffStartHours[1])
				values[0] += value;
			else if (h >= data.settings.tariffStartHours[1] && h < data.settings.tariffStartHours[2])
				values[1] += value;
			else if (h >= data.settings.tariffStartHours[2] || h < data.settings.tariffStartHours[0])
				values[2] += value;
		}
		$(`.cs-last-24-hours-monitor-${m + 1}-total span`).html(`${total.toFixed(2)} kWh`);
		$(`.cs-last-24-hours-monitor-${m + 1}-average span`)
			.html(`${(total / 24).toFixed(2)} kWh (${values.map(function (a) { return (a / 24).toFixed(2); }).join("/")})`);
		if (total == 0) total = 1;
		// by tariffs
		for (var t = 0; t < data.tariffsCount; t++) {
			$(`.cs-last-24-hours-monitor-${m + 1}.tariff-${t + 1} span`)
				.html(`${values[t].toFixed(2)} kWh (${Math.round(values[t] / total * 100)} %)`);
		}
	}

	//// Last month per monitor
	for (var m = 0; m < data.monitorsCount; m++) {
		if (data.settings.monitorsNames[m] != "") {
			$(`.section-title.last-month-monitor-${m + 1} span`).html(` (${data.settings.monitorsNames[m]})`);
		}

		// total
		var total = 0;
		var values = [];
		var dt = new Date(data.time);
		var daysCount = new Date(dt.getUTCFullYear(), dt.getUTCMonth(), 0).getDate(); // get previous month length
		for (var t = 0; t < data.tariffsCount; t++) {
			values.push(0);
			for (var d = 0; d < daysCount; d++) {
				values[t] += toKilo(data.days[m][t][d]);
			}
			total += values[t];
		}
		$(`.cs-last-month-monitor-${m + 1}-total span`).html(`${total.toFixed(2)} kWh`);
		$(`.cs-last-month-monitor-${m + 1}-average span`)
			.html(`${(total / daysCount).toFixed(2)} kWh (${values.map(function (a) { return (a / daysCount).toFixed(2); }).join("/")})`);
		if (total == 0) total = 1;
		// by tariffs
		for (var t = 0; t < data.tariffsCount; t++) {
			var value = 0;
			for (var d = 0; d < daysCount; d++) {
				value += toKilo(data.days[m][t][d]);
			}
			$(`.cs-last-month-monitor-${m + 1}.tariff-${t + 1} span`)
				.html(`${value.toFixed(2)} kWh (${Math.round(value / total * 100)} %)`);
		}
	}


	// History
	//// Year
	var total = 0;
	for (var t = 0; t < data.tariffsCount; t++) {
		total += lastYear(data, t);
	}
	$(".ch-year-total.all-monitors span").html(`${total.toFixed(2)} kWh`);
	$(".ch-month-average.all-monitors span").html(`${(total / 12).toFixed(2)} kWh`);
	if (total == 0) total = 1;
	var values = [];
	for (var t = 0; t < data.tariffsCount; t++) {
		values[t] = lastYear(data, t);
		$(`.ch-year-tariff-${t + 1}.all-monitors span`)
			.html(`${values[t].toFixed(2)} kWh (${Math.round(values[t] / total * 100)} %)`);
	}
	$(".ch-month-average.all-monitors span").html(` (${values.map(function (a) { return (a / 12).toFixed(2); }).join("/")})`)

	//// Year per monitor
	for (var m = 0; m < data.monitorsCount; m++) {
		if (data.settings.monitorsNames[m] != "") {
			$(`.section-title.year-monitor-${m + 1} span`).html(` (${data.settings.monitorsNames[m]})`);
		}

		var total = 0;
		for (var t = 0; t < data.tariffsCount; t++) {
			total += lastYear(data, t, m);
		}
		$(`.ch-year-total.year-monitor-${m + 1} span`).html(`${total.toFixed(2)} kWh`);
		$(`.ch-month-average.year-monitor-${m + 1} span`).html(`${(total / 12).toFixed(2)} kWh`);
		if (total == 0) total = 1;
		var values = [];
		for (var t = 0; t < data.tariffsCount; t++) {
			values[t] = lastYear(data, t, m);
			$(`.ch-year-tariff-${t + 1}.year-monitor-${m + 1} span`)
				.html(`${values[t].toFixed(2)} kWh(${Math.round(values[t] / total * 100)} %)`);
		}
		$(`.ch-month-average.year-monitor-${m + 1} span`).html(` (${values.map(function (a) { return (a / 12).toFixed(2); }).join("/")})`)
	}
});

function InitializeSwiper() {
	// Initialize Swiper
	if (window.swiper)
		return;
	window.swiper = new Swiper('.swiper-container', {
		slidesPerView: 1,
		spaceBetween: 30,
		keyboard: {
			enabled: true,
		},
		pagination: {
			el: '.swiper-pagination',
			clickable: true,
		},
		navigation: {
			nextEl: '.swiper-button-next',
			prevEl: '.swiper-button-prev',
		},
	});
}

// Current
function drawTotalConsumption(canvasName) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;

		// sum by tariffs
		var values = [];
		for (var t = 0; t < data.tariffsCount; t++) {
			values.push(0);
			if (t < data.tariffsCount - 1 && data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1])
				continue;
			for (var m = 0; m < data.monitorsCount; m++) {
				values[t] += toKilo(data.current.month[m][t]);
			}
		}

		clearCanvas(canvasName);
		drawDonutChart(
			canvasName,
			values,
			{
				label: "Total",
				units: "kWh",
				colors: ["#ec96a4", "#63b59c", "#68829e"]
			});
	});
}

function drawCurrentUsage(canvasName) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;

		var maxValue = Math.max.apply(Math, data.current.energy);
		var values = [];
		for (var m = 0; m < data.monitorsCount; m++) {
			if (!isMonitorVisible(data, m))
				continue;
			values.push(data.current.energy[m]);
		}

		clearCanvas(canvasName);
		drawCircularChart(
			canvasName,
			values,
			maxValue * 1.2,
			{
				radius: 110,
				label: "Total",
				units: "W"
			});
	});
}

function drawLast24Hours(canvasName, monitorIdx) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;

		var labels = [];
		var values = [];
		var colors = [];
		var dt = new Date(data.time);
		for (var h = 0; h < 24; h++) {
			var hour = (dt.getUTCHours() + h) % 24;
			labels.push(hour);
			values.push(0);
			if (monitorIdx != undefined) {
				values[h] = toKilo(data.hours[monitorIdx][hour]);
			}
			else {
				for (var m = 0; m < data.monitorsCount; m++) {
					values[h] += toKilo(data.hours[m][hour]);
				}
			}

			if (hour >= data.settings.tariffStartHours[0] && hour < data.settings.tariffStartHours[1])
				colors.push("#ec96a4");
			else if (hour >= data.settings.tariffStartHours[1] && hour < data.settings.tariffStartHours[2])
				colors.push("#63b59c");
			else if (hour >= data.settings.tariffStartHours[2] || hour < data.settings.tariffStartHours[0])
				colors.push("#68829e");
		}

		clearCanvas(canvasName);
		drawBarChart(
			canvasName,
			labels,
			values,
			{ colors: colors });
	});
}

function drawLastMonth(canvasName, monitorIdx) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;
			
		var labels = [];
		var dt = new Date(data.time);
		var daysCount = new Date(dt.getUTCFullYear(), dt.getUTCMonth(), 0).getDate(); // get previous month length
		for (var d = 0; d < daysCount; d++) {
			var dt2 = new Date(dt);
			dt2.setDate(dt.getDate() - daysCount + d);
			labels.push(dt2.getUTCDate());
		}

		var values = [];
		for (var t = 0; t < data.tariffsCount; t++) {
			values.push([]);
			for (var d = 0; d < daysCount; d++) {
				var dt2 = new Date(dt);
				dt2.setDate(dt.getDate() - daysCount + d);

				values[t].push(0);
				if (monitorIdx != undefined) {
					values[t][d] = toKilo(data.days[monitorIdx][t][dt2.getUTCDate() - 1]);
				}
				else {
					for (var m = 0; m < data.monitorsCount; m++) {
						values[t][d] += toKilo(data.days[m][t][dt2.getUTCDate() - 1]);
					}
				}
			}
		}
		values.reverse();

		clearCanvas(canvasName);
		drawLineChart(
			canvasName,
			labels,
			values);
	});
}

// Sections
function drawCurrentHour(canvasName) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;

		var values = [];
		for (var m = 0; m < data.monitorsCount; m++) {
			if (!isMonitorVisible(data, m))
				continue;
			values.push(toFloat(data.current.hour[m]));
		}

		clearCanvas(canvasName);
		drawDonutChart(
			canvasName,
			values,
			{
				label: "Total",
				units: "Wh"
			});
	});
}

function drawCurrentDay(canvasName) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;
			
		var labels = [];
		var values = [];
		for (var m = 0; m < data.monitorsCount; m++) {
			if (!isMonitorVisible(data, m))
				continue;
			labels.push("Monitor " + (m + 1));
			values.push([]);
			for (var t = 0; t < data.tariffsCount; t++) {
				if (t < data.tariffsCount - 1 && data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1])
					continue;
				values[m].push(toKilo(data.current.day[m][t]));
			}
		}

		clearCanvas(canvasName);
		drawVerticalMultiBarsChart(
			canvasName,
			labels,
			values,
			{ units: "kWh" });
	});
}

// History
function drawLastYear(canvasName, monitorIdx) {
	$(window).on("data", function (e, data) {
		if (document.getElementById(canvasName) == null)
			return;
			
		// sum by tariffs
		var values = [];
		for (var t = 0; t < data.tariffsCount; t++) {
			values.push([]);
			for (var i = 0; i < 12; i++) {
				values[t].push(0);
				if (t < data.tariffsCount - 1 && data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1])
					continue;

				if (monitorIdx != undefined) {
					values[t][i] = toKilo(data.months[monitorIdx][t][i]);
				}
				else {
					for (var m = 0; m < data.monitorsCount; m++) {
						values[t][i] += toKilo(data.months[m][t][i]);
					}
				}
			}
		}
		var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Noe", "Dec"];

		// shift array so values to be ordered backward from last month
		for (var m = 0; m < getBillMonth(data) - 1; m++) {
			months.push(months[0]);
			months.shift();
			for (var t = 0; t < data.tariffsCount; t++) {
				if (t < data.tariffsCount - 1 && data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1])
					continue;
				values[t].push(values[t][0]);
				values[t].shift();
			}
		}
		months.reverse();
		for (var t = 0; t < data.tariffsCount; t++) {
			if (t < data.tariffsCount - 1 && data.settings.tariffStartHours[t] == data.settings.tariffStartHours[t + 1])
				continue;
			values[t].reverse();
		}

		clearCanvas(canvasName);
		drawVerticalSplitBarChart(
			canvasName,
			months,
			values,
			{ units: "kWh" }
		);
	});
}

function importSettings() {
	loadFromFile(".json", content => {
		if (confirm("Are you sure?")) {
			$.post("/settings", JSON.parse(content), () => location.reload());
		}
	});
}

function exportSettings() {
	$.get("/data.json", (json) => {
		var settings = JSON.stringify(json["settings"], null, 2);
		saveToFile("settings.json", settings);
	});
}

function importData() {
	loadFromFile(".json", content => {
		if (confirm("Are you sure?")) {
			$.ajax({
				'type': 'POST',
				'url': "/import",
				'contentType': 'application/json',
				'data': content,
				'dataType': 'json',
				function() { location.reload(); }
			});
		}
	});
}

function exportData() {
	$.get("/data.json", (json) => {
		var data = JSON.stringify({ "hours": json["hours"], "days": json["days"], "months": json["months"] }, null, 2)
		saveToFile("data.json", data);
	});
}


function toFloat(value) {
	return value / 100.0;
}

function toKilo(value) {
	return toFloat(value) / 1000.0;
}

function getBillMonth(data) {
	var dt = new Date(data.time);
	var month = dt.getUTCMonth() + 1;
	if (dt.getUTCDate() < data.settings.billDay)
		month--;
	if (month < 1)
		month += 12;
	return month;
}

function lastYear(data, tariffIdx, monitorIdx) {
	var result = 0;
	for (var i = 0; i < 12; i++) {
		if (monitorIdx != undefined) {
			result += toKilo(data.months[monitorIdx][tariffIdx][i]);
		}
		else {
			for (var m = 0; m < data.monitorsCount; m++) {
				result += toKilo(data.months[m][tariffIdx][i]);
			}
		}
	}
	return result;
}

function isMonitorVisible(data, monitorIdx) {
	for (var m = 0; m < data.monitorsCount; m++) {
		if (data.settings.monitorsNames[m] != "" && data.settings.monitorsNames[monitorIdx] == "")
			return false;
	}
	return true; // if monitor name isn't empty or all monitor names are empty
}

function clearCanvas(canvasName) {
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");
	ctx.clearRect(0, 0, canvas.width, canvas.height);
}

function saveToFile(filename, content) {
	var a = document.createElement("a");
	var file = new Blob([content], {type: "text/plain"});
	a.href = URL.createObjectURL(file);
	a.download = filename;
	a.click();
}

function loadFromFile(extension, handler) {
	var input = document.createElement("input");
	input.type = "file";
	input.accept = extension;
	input.addEventListener('change', function (e) {
		var file = e.target.files[0];
		var reader = new FileReader();
		reader.onload = function (evt) {
			handler(evt.target.result);
		}
		reader.readAsText(file)
	}, false);
	input.click();
}