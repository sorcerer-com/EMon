function drawCircularChart(canvasName, values, maxValue, options) {

	// default values
	options = options || {};
	options.width = options.width || 10;
	options.colors = options.colors || ["#a1d6e2", "#1995ad", "#0f5b68", "#626d71"];
	options.textColor = options.textColor || "#828282";
	options.font = options.font || "14px Arial";
	options.label = options.label || "Label";
	options.units = options.units || "units";

	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");
	ctx.lineWidth = options.width;

	var centerX = canvas.width / 2;
	var centerY = canvas.height / 2;

	if (!options.radius) {
		options.radius = canvas.width / 2.5;
	}

	var total = 0;
	for (var i = 0; i < values.length; i++) {
		len = values[i] / maxValue;
		ctx.beginPath();
		ctx.strokeStyle = options.colors[i];
		ctx.arc(centerX, centerY, options.radius - (i * (options.width + 2)), Math.PI / 2, (len * Math.PI * 2) + Math.PI / 2); // (x, y, R, startAngle, endAngle)
		ctx.stroke();

		total += values[i];
	}

	// text
	ctx.font = options.font;
	ctx.fillStyle = options.textColor;
	var textWidth = ctx.measureText(options.label).width;
	ctx.fillText(options.label, centerX - textWidth / 2, centerY - 2);
	var textWidth = ctx.measureText(Math.floor(total) + " " + options.units).width;
	ctx.fillText(Math.floor(total) + " " + options.units, centerX - textWidth / 2, centerY + parseInt(ctx.font));
}

function drawBarChart(canvasName, labels, values, options) {

	// default values
	options = options || {};
	options.font = options.font || "14px Arial";
	options.textColor = options.textColor || "#bcbabe";
	options.scaleYColor = options.scaleYColor || "#bcbabe";
	options.colors = options.colors || ["#9a9eab", "#ec96a4"];
	options.interactive = options.interactive || true;
	options.highlightColor = options.highlightColor || "#d0d6e5";
	options.highlightTextColor = options.highlightTextColor || "#494849";

	// chart
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");
	ctx.font = options.font;
	var textHeight = parseInt(ctx.font);

	var barsCount = values.length;
	var max = Math.ceil(Math.max.apply(Math, values));
	var coef = (canvas.height - 21 - textHeight / 2) / max;

	var barWidth = Math.floor((canvas.width - 22 - barsCount - 1) / barsCount); // (canvas.width - barsStartX - gapsCount) / hours
	var endChartX = (barWidth * barsCount) + barsCount - 1 + 22 - 16 + 4; // values + gapsCount + barsStartX - chartScaleStartX - 1

	// chart scale
	ctx.fillStyle = options.scaleYColor;
	ctx.fillText("0", 3, canvas.height - textHeight / 2);
	ctx.fillRect(17, canvas.height - textHeight, endChartX, 1); // (x, y, width, height)	

	ctx.fillText(Math.floor(max / 2), 0, Math.floor((canvas.height + 13) / 2 - 1));
	ctx.fillRect(17, Math.floor((canvas.height + 13 - textHeight) / 2), endChartX, 1);

	ctx.fillText(Math.floor(max), 0, 20);
	ctx.fillRect(17, 20 - textHeight / 2, endChartX, 1);


	// bars
	var textBetween = Math.ceil(ctx.measureText("00").width / barWidth);
	for (var i = 0; i < barsCount; i++) {
		ctx.fillStyle = options.colors[i % options.colors.length];
		ctx.fillRect(i * (barWidth + 1) + 22, canvas.height - textHeight, barWidth, -Math.floor(values[i] * coef) - 1);

		if (i % textBetween == 0) {
			ctx.fillStyle = options.textColor;
			ctx.font = "italic bold " + options.font;
			var textX = i * (barWidth + 1) + 22 + barWidth / 2 - ctx.measureText(labels[i]).width / 2;
			ctx.fillText(labels[i], textX, canvas.height);
		}
	}

	if (options.interactive) {
		canvas.onmousemove = function (e) {
			var posX = e.clientX - canvas.getBoundingClientRect().left;
			var posY = e.clientY - canvas.getBoundingClientRect().top;

			if (ctx.redraw) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);

				var optionsCopy = options;
				optionsCopy.interactive = false;
				drawBarChart(canvasName, labels, values, optionsCopy);

				ctx.redraw = undefined;
			}

			var idx = Math.floor((posX - 22) / (barWidth + 1));
			if (idx < 0 || idx >= barsCount)
				return;

			if (posY > canvas.height - textHeight ||
				posY < canvas.height - textHeight - Math.floor(values[idx] * coef) - 1)
				return;

			ctx.redraw = true;
			ctx.fillStyle = options.highlightColor;
			ctx.fillRect(idx * (barWidth + 1) + 22, canvas.height - textHeight, barWidth, -Math.floor(values[idx] * coef) - 1);

			ctx.fillStyle = options.highlightTextColor;
			var textX = idx * (barWidth + 1) + 22 + barWidth / 2 - ctx.measureText(values[idx].toFixed(2)).width / 2;
			var textY = canvas.height - textHeight - Math.floor(values[idx] * coef) - 3;
			ctx.fillText(values[idx].toFixed(2), textX, textY);

			ctx.font = "italic bold " + options.font;
			var textX = idx * (barWidth + 1) + 22 + barWidth / 2 - ctx.measureText(labels[idx]).width / 2;
			ctx.fillText(labels[idx], textX, canvas.height);
		};
		canvas.onmouseleave = function (e) {
			if (ctx.redraw) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);

				var optionsCopy = options;
				optionsCopy.interactive = false;
				drawBarChart(canvasName, labels, values, optionsCopy);

				ctx.redraw = undefined;
			}
		};
	}
}

function drawLineChart(canvasName, labels, values, options) {

	// default values
	options = options || {}
	options.font = options.font || "14px Arial";
	options.scaleWidth = options.scaleWidth || 5;
	options.backgroundColor = options.backgroundColor || "#ffeedd";
	options.scaleYColor = options.scaleYColor || "#bcbabe";
	options.scaleXColor = options.scaleXColor || "#ffffff";
	options.textColor = options.textColor || "#bcbabe";
	options.colors = options.colors || ["#5282fc", "#63b59c", "#ec96a4"];
	options.interactive = options.interactive || true;
	options.highlightColor = options.highlightColor || "#a2a7b2";
	options.highlightTextColor = options.highlightTextColor || "#7e7c7f";

	// chart
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");
	var gridHeight = canvas.height - 10;
	var gridWidth = canvas.width - 18;

	var maxs = [];
	for (var j = 0; j < values.length; j++) {
		maxs.push(Math.max.apply(Math, values[j]));
	}
	var max = Math.max.apply(Math, maxs);

	var maxCount = Math.max.apply(Math, values.map(function (a) { return a.length; }));
	var coef = (gridHeight - 25) / max;

	var barWidth = Math.floor(gridWidth / maxCount);

	// chart scale background
	ctx.font = options.font;
	var textHeight = parseInt(ctx.font);

	ctx.fillStyle = options.backgroundColor;
	ctx.fillRect(18, 0, barWidth * (maxCount - 1), gridHeight - textHeight / 2);

	// chart scale grid		
	for (var scaleX = 0; scaleX < maxCount; scaleX += options.scaleWidth) {
		ctx.fillStyle = options.scaleXColor;
		ctx.fillRect(18 + (scaleX * barWidth), 0, 2, gridHeight - textHeight / 2);
		ctx.fillStyle = options.textColor;
		var textWidth = ctx.measureText(labels[scaleX]).width;
		ctx.fillText(labels[scaleX], 19 - textWidth / 2 + scaleX * barWidth, canvas.height);
	}

	ctx.fillStyle = options.scaleYColor;
	ctx.fillText("0", 3, canvas.height - textHeight);
	ctx.fillRect(17, gridHeight - textHeight / 2, barWidth * (maxCount - 1), 1); // (x, y, width, height)	

	ctx.fillText(Math.floor(((gridHeight / 5) * 2) / coef), 0, Math.floor(gridHeight / 5) * 3 + textHeight / 2 - 1);
	ctx.fillRect(17, Math.floor(gridHeight / 5) * 3, barWidth * (maxCount - 1), 1);

	ctx.fillText(Math.floor(((gridHeight / 5) * 4) / coef), 0, Math.floor(gridHeight / 5) + textHeight / 2 - 1);
	ctx.fillRect(17, Math.floor(gridHeight / 5), barWidth * (maxCount - 1), 1);

	// lines
	for (var j = 0; j < values.length; j++) {
		var grd = ctx.createLinearGradient(0, 0, 0, gridHeight);
		grd.addColorStop(0, options.colors[j % options.colors.length] + "aa");
		grd.addColorStop(1, options.colors[j % options.colors.length] + "00");
		ctx.fillStyle = grd;

		ctx.beginPath();
		ctx.strokeStyle = options.colors[j % options.colors.length];
		ctx.moveTo(18, gridHeight - textHeight / 2);
		var points = [];
		for (i = 0; i < values[j].length; i++) {
			points.push(18 + (i * barWidth), gridHeight - textHeight / 2 - Math.floor(values[j][i] * coef));//(Math.floor(Math.random() * (90 - 30)) + 30));
		}
		ctx.curve(points);
		ctx.lineTo(18 + ((i - 1) * barWidth), gridHeight - textHeight / 2);
		ctx.fill();
		ctx.stroke();
	}

	if (options.interactive) {
		canvas.onmousemove = function (e) {
			var posX = e.clientX - canvas.getBoundingClientRect().left;
			var posY = e.clientY - canvas.getBoundingClientRect().top;

			if (ctx.redraw) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);

				var optionsCopy = options;
				optionsCopy.interactive = false;
				drawLineChart(canvasName, labels, values, optionsCopy);

				ctx.redraw = undefined;
			}

			var idx = Math.floor((posX - 18) / barWidth + 0.5);
			if (idx < 0 || idx >= maxCount)
				return;

			ctx.redraw = true;

			for (var j = 0; j < values.length; j++) {
				ctx.fillStyle = options.colors[j % options.colors.length];
				var pointY = gridHeight - textHeight / 2 - Math.floor(values[j][idx] * coef) - 2;
				ctx.fillRect(18 + idx * barWidth - 2, pointY, 5, 5);

				var textX = 19 + idx * barWidth - ctx.measureText(values[j][idx].toFixed(2)).width / 2;
				var textY = canvas.height - textHeight - 10 - Math.floor(values[j][idx] * coef);
				ctx.fillText(values[j][idx].toFixed(2), textX, textY);
			}

			ctx.font = "italic bold " + options.font;
			ctx.fillStyle = options.textColor;
			var textX = 19 + idx * barWidth - ctx.measureText(labels[idx]).width / 2;
			ctx.fillText(labels[idx], textX, canvas.height);
		};
		canvas.onmouseleave = function (e) {
			if (ctx.redraw) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);

				var optionsCopy = options;
				optionsCopy.interactive = false;
				drawLineChart(canvasName, labels, values, optionsCopy);

				ctx.redraw = undefined;
			}
		};
	}
}

function drawVerticalSplitBarChart(canvasName, labels, values, options) {

	// default values
	options = options || {}
	options.font = options.font || "14px Arial";
	options.textColor = options.textColor || "#bcbabe";
	options.text2Color = options.text2Color || "#555555";
	options.barColors = options.barColors || ["#e5c912", "#afdd3b", "#68829e"];
	options.units = options.units || "units";

	// chart
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");

	var maxCount = Math.max.apply(Math, values.map(function (a) { return a.length; }));
	var gridWidth = canvas.width - (27 + 47);
	var gridHeight = canvas.height;

	var sum = [];
	for (var i = 0; i < maxCount; i++) {
		var temp = 0;
		for (var j = 0; j < values.length; j++)
			temp += values[j][i];
		sum.push(temp);
	}

	var barHeight = Math.floor(gridHeight / maxCount);
	var blankSpaceHeight = Math.floor(barHeight / 4);

	var max = Math.max(Math.max.apply(Math, sum), 1);
	var coef = gridWidth / max;

	ctx.font = options.font;
	for (var i = 0; i < maxCount; i++) {
		// label
		ctx.fillStyle = options.textColor;
		ctx.fillText(labels[i], 0, barHeight * i + barHeight / 2);

		var startX = 27;
		for (var j = 0; j < values.length; j++) {
			// bar parts
			var grd = ctx.createLinearGradient(startX, 0, startX + values[j][i] * coef + 1, 0);
			grd.addColorStop(0, options.barColors[j % options.barColors.length]);
			grd.addColorStop(1, "#ffffff");
			ctx.fillStyle = grd;
			ctx.fillRect(startX, (barHeight * i), values[j][i] * coef + 1, barHeight - blankSpaceHeight);
			startX += values[j][i] * coef;

			ctx.fillStyle = options.text2Color;
			ctx.fillText(values[j][i].toFixed(2), startX - values[j][i] * coef + 5, barHeight * i + barHeight / 2 + 2);
		}

		// ratio
		// var ratioText = values.map(function(a) { return a[i].toFixed(2); }).join(" / ");
		// var textWidth = ctx.measureText(ratioText).width;
		// ctx.fillStyle = options.text2Color;
		// ctx.fillText(ratioText, (canvas.width - 27) / 2 - textWidth / 2, barHeight * i + barHeight / 2);

		// sum
		var textWidth = ctx.measureText(sum[i].toFixed(2) + " " + options.units).width;
		ctx.fillStyle = options.textColor;
		ctx.fillText(sum[i].toFixed(2) + " " + options.units, canvas.width - textWidth, barHeight * i + barHeight / 2);
	}
}


function drawVerticalMultiBarsChart(canvasName, labels, values, options) {

	// default values
	options = options || {}
	options.font = options.font || "14px Arial";
	options.textColor = options.textColor || "#bcbabe";
	options.colors = options.colors || ["#ec96a4", "#63b59c", "#68829e"];
	options.units = options.units || "units";

	// chart
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");

	var maxCount = Math.max.apply(Math, values.map(function (a) { return a.length; }));
	var maxLabelWidth = Math.max.apply(Math, labels.map(function (a) { return ctx.measureText(a).width * 1.6; }));
	var gridWidth = canvas.width - maxLabelWidth - 65;
	var gridHeight = canvas.height;

	var barHeight = Math.floor((gridHeight - values.length) / values.length / maxCount) - 3;
	var blankSpaces = values.length * 3;

	var maxs = [];
	for (var j = 0; j < values.length; j++) {
		maxs.push(Math.max.apply(Math, values[j]));
	}
	var max = Math.max.apply(Math, maxs);
	var coef = gridWidth / max;

	ctx.font = options.font;
	var textHeight = parseInt(ctx.font);
	for (var i = 0; i < values.length; i++) {
		// label
		ctx.fillStyle = options.textColor;
		ctx.fillText(labels[i], 0, (barHeight * maxCount + blankSpaces) * i + barHeight * maxCount / 2 + textHeight / 2);

		for (var j = 0; j < maxCount; j++) {
			ctx.fillStyle = options.colors[j % options.colors.length];
			ctx.fillRect(maxLabelWidth, (barHeight * maxCount + blankSpaces) * i + barHeight * j + j, values[i][j] * coef + 1, barHeight);

			ctx.fillStyle = options.textColor;
			ctx.fillText(values[i][j].toFixed(2) + " " + options.units, values[i][j] * coef + maxLabelWidth + 4, (barHeight * maxCount + blankSpaces) * i + barHeight * j + j + barHeight / 2 + textHeight / 2 - 2);
		}
	}

}

function drawDonutChart(canvasName, sections, options) {

	// default values
	options = options || {}
	options.sectionsWidth = options.sectionsWidth || 0;
	options.sectionsRadius = options.sectionsRadius || 0;
	options.colors = options.colors || ["#626d71", "#cdcdc0", "#ddbc95", "#b38867"];
	options.textColor = options.textColor || "#828282";
	options.font = options.font || "14px Arial";
	options.label = options.label || "Label";
	options.units = options.units || "units";

	// chart
	var canvas = document.getElementById(canvasName);
	var ctx = canvas.getContext("2d");

	var total = 0;
	for (var i = 0; i < sections.length; i++) {
		total += sections[i];
	}
	var degree = 2 / total;
	var blankSpace = 3 * Math.PI / 180;

	var centerX = canvas.width / 2;
	var centerY = canvas.height / 2;

	if (options.sectionsWidth == 0) {
		options.sectionsWidth = canvas.width / 12;
	}
	if (options.sectionsRadius == 0) {
		options.sectionsRadius = canvas.width / 2.5;
	}

	ctx.lineWidth = options.sectionsWidth;
	var startAngle = (1 / 2) / degree;
	for (var i = 0; i < sections.length; i++) {
		if (sections[i] * degree * Math.PI < blankSpace)
			continue;

		ctx.beginPath();
		ctx.strokeStyle = options.colors[i];
		ctx.arc(centerX, centerY, options.sectionsRadius, startAngle * degree * Math.PI, (sections[i] + startAngle) * degree * Math.PI - blankSpace); // (x, y, R, startAngle, endAngle)
		ctx.stroke();

		startAngle += sections[i];
	}

	// text
	ctx.font = options.font;
	ctx.fillStyle = options.textColor;
	var textWidth = ctx.measureText(options.label).width;
	ctx.fillText(options.label, centerX - textWidth / 2, centerY - 2);
	var textWidth = ctx.measureText(total.toFixed(2) + " " + options.units).width;
	ctx.fillText(total.toFixed(2) + " " + options.units, centerX - textWidth / 2, centerY + parseInt(ctx.font));
}