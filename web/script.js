/* Karadio - Optimized Unified Stylesheet */
/* Combined style.css and style1.css with CSS variables for theme switching */

/* ===== CSS Variables for Easy Theme Switching ===== */
:root {
	/* Light theme (default) */
	--bg-color: #033069;
	--text-color: #222;
	--header-bg: #39c;
	--header-gradient: radial-gradient(#39c, #033069);
	--content-bg: #f4f4f4;
	--content-text: #222;
	--border-color: #185213;
	--table-even: #f8f8f8;
	--table-odd: #f0f0f0;
	--input-bg: #f8f8ff;
	--button-bg: #f8f8ff;
	--label-bg: #39c;
	--label-hover: #3cc;
	--label-active: #38c;
	--link-color: #222;
	--box-shadow: 10px 5px 10px 0 #333, -10px 6px 10px 0 #333;
}

body.dark-theme {
	/* Dark theme */
	--bg-color: #333333;
	--text-color: #ffffff;
	--header-bg: #333333;
	--header-gradient: radial-gradient(#111111, #333333);
	--content-bg: #111111;
	--content-text: #ffffff;
	--border-color: #6d6f6d;
	--table-even: #151515;
	--table-odd: #0d0d0d;
	--input-bg: #E8C37D;
	--button-bg: #554;
	--label-bg: #666145;
	--label-hover: #96906B;
	--label-active: #918B69;
	--link-color: #AAAAAA;
	--box-shadow: 0px 0px 9px 2px #1E1C1C;
}

/* ===== Global Styles ===== */
html, body {
	margin: 0;
	height: 100%;
	padding: 0;
	font: 12px "Trebuchet MS", sans-serif;
	color: var(--text-color);
	background-color: var(--bg-color);
}

#MAIN {
	width: 100%;
	overflow: hidden;
	margin: 0;
}

/* ===== Header Styles ===== */
#HEADER {
	z-index: 10;
	position: fixed;
	top: 0;
	width: 100%;
	margin: 0 auto;
}

#HEAD {
	width: 100%;
	min-height: 55px;
	text-align: center;
	padding: 20px 0 5px 20px;
	background-color: var(--header-bg);
	background-image: var(--header-gradient);
	color: #f0f0ff;
	font-size: 22px;
	line-height: 25px;
	box-shadow: 0 10px 10px 0 #333;
}

#HEAD span.bold4 {
	font-size: 22px;
	width: 98%;
	display: inline;
	animation: mymove4 1s infinite alternate;
}

@keyframes mymove4 {
	from { color: #fff; }
	to { color: #ff0; }
}

div.RIGHT {
	float: right;
	padding: 15px 0 0 5px;
	margin-right: 20px;
	line-height: 15px;
}

div.RIGHT a, a:visited {
	font-size: 10px;
	line-height: 10px;
	color: #f0f0ff;
	text-decoration: none;
}

div.RIGHT div {
	font-size: 10px;
	line-height: 15px;
	color: #fff0f0;
}

div.LEFT {
	float: left;
	padding: 10px 3px 0 15px;
}

/* ===== Content Styles ===== */
.CONTENT {
	width: 92%;
	position: relative;
	overflow: hidden;
	min-height: calc(100% - 70px);
	margin: auto;
	padding: 10px 10px 0 10px;
	padding-top: 30px;
	background-color: var(--content-bg);
	box-shadow: var(--box-shadow);
}

.CONTENT h1 {
	font: 18px "Trebuchet MS", sans-serif;
	color: #f8f8f8;
	background-color: var(--label-bg);
	padding: 4px;
}

.CONTENT p {
	font-size: 15px;
	margin: 2px 10px 5px 10px;
	display: inline-block;
}

.CONTENT a, a:visited {
	font-size: 15px;
	color: var(--link-color);
	text-decoration: none;
}

.CONTENT a:hover {
	color: #666;
}

.CONTENT span.bold {
	font-weight: 900;
}

.CONTENT form {
	font: 15px "Trebuchet MS", sans-serif;
	width: 98%;
}

.CONTENT iframe {
	height: 400px;
	width: 100%;
}

/* ===== Tabs Styling ===== */
.tabs span.bold {
	height: 25px;
	font-weight: 900;
	width: 140px;
	display: inline-block;
}

.tabs span.bold1 {
	font-weight: 900;
	width: 100px;
	display: inline-block;
}

.tabs span.bold2 {
	font-weight: 900;
	width: 50px;
	display: inline-block;
}

.tabs span.bold3 {
	font-size: 22px;
	width: 98%;
	display: inline;
	animation: mymove 1s infinite alternate;
}

.tabs span.bold4 {
	width: 98%;
	display: inline-block;
}

@keyframes mymove {
	from { color: #0; }
	to { color: #00f; }
}

.tabs {
	width: calc(100% - 20px);
	float: none;
	list-style: none;
	position: relative;
	text-align: left;
}

ul#ordered li { float: none; text-align: left; }

.tabs li { float: left; }

.tabs hr { height: 1px; color: var(--border-color); }

.tabs input[type="radio"] {
	position: absolute;
	top: -9999px;
	left: -9999px;
}

/* ===== Range Input Styling ===== */
.tabs input[type=range] {
	min-width: 200px;
	background-color: var(--input-bg);
	background: linear-gradient(var(--input-bg), #e8e8ef);
	border-radius: 5px;
	border: 1px solid #444;
}

.tabs input[type=range]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

/* ===== Text Input Styling ===== */
.tabs input[type="text"] {
	font-size: 12px;
	width: 140px;
	height: 20px;
	color: var(--content-text);
	background-color: var(--input-bg);
	background: linear-gradient(var(--input-bg), #e8e8ef);
	border-radius: 5px;
	border: 1px solid #444;
}

.tabs input[type="text"]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

.tabs [id^="sw"] input[type="text"] {
	font-size: 14px;
	min-width: 70px;
	height: 20px;
}

.tabs [id^="sw"] input[type="text"]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

/* ===== Password Input Styling ===== */
.tabs input[type="password"] {
	font-size: 14px;
	width: 140px;
	height: 20px;
	color: var(--content-text);
	background-color: var(--input-bg);
	background: linear-gradient(var(--input-bg), #e8e8ef);
	border-radius: 5px;
	border: 1px solid #444;
}

.tabs input[type="password"]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

/* ===== File Input Styling ===== */
.tabs input[type="file"] {
	font-size: 14px;
	min-width: 140px;
	height: 30px;
	color: var(--content-text);
	background-color: var(--input-bg);
	background: linear-gradient(var(--input-bg), #e8e8ef);
	border-radius: 5px;
	border: 1px solid #444;
}

.tabs input[type="file"]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

/* ===== Button Styling ===== */
.tabs input[type="button"] {
	font-size: 14px;
	min-width: 100px;
	height: 25px;
	color: var(--content-text);
	background-color: var(--button-bg);
	background: linear-gradient(var(--button-bg), #555);
	border-radius: 5px;
	border: 1px solid #444;
	cursor: pointer;
}

.tabs input[type="button"]:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

.tabs [id^="sw"] input[type="button"] {
	font-size: 14px;
	min-width: 50px;
	height: 25px;
}

.tabs span input[type="button"] {
	font-size: 14px;
	min-width: 100px;
	height: 30px;
}

/* ===== Select Styling ===== */
.tabs select {
	min-width: 230px;
	color: var(--content-text);
	background-color: var(--button-bg);
	background: linear-gradient(var(--button-bg), #555);
	border: 1px solid #444;
	border-radius: 5px;
}

.tabs select:hover {
	box-shadow: 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

.tabs option {
	min-width: 230px;
	color: var(--content-text);
	background-color: var(--button-bg);
	border: 1px solid #444;
	border-radius: 5px;
}

/* ===== Label Styling ===== */
.tabs label {
	display: run-in;
	padding: 14px;
	font-weight: normal;
	text-transform: uppercase;
	background: var(--label-bg);
	cursor: pointer;
	position: relative;
	top: 4px;
	transition: all 0.2s ease-in-out;
	border-radius: 5px;
}

.tabs label:hover {
	background: var(--label-hover);
}

.tabs span.coutput label {
	display: run-in;
	padding: 1px;
	background: var(--label-bg);
	top: 1px;
	border-radius: 0;
}

.tabs [id^="output"]:checked + label {
	top: 0;
	padding: 2px;
	color: #EEE;
	background: var(--label-active);
	border-radius: 4px;
}

/* ===== Tab Content ===== */
.tabs .tab-content {
	z-index: 2;
	display: none;
	overflow: auto;
	width: 100%;
	font-size: 14px;
	position: absolute;
	top: 53px;
	left: 0;
	border-color: var(--border-color);
}

.tabs [id^="tab"]:checked + label {
	top: 0;
	padding: 16px 14px;
	font-size: 18px;
	color: #EEE;
	background: var(--label-active);
	border-radius: 5px;
}

.tabs [id^="tab"]:checked ~ [id^="tab-content"] {
	display: block;
}

/* ===== Table Styling ===== */
table#stationsTable,
table#stationsTable th,
table#stationsTable td {
	border: solid 1px var(--border-color);
	border-collapse: collapse;
	width: 100%;
	overflow-x: hidden;
}

table#stationsTable tr:nth-child(even) {
	background-color: var(--table-even);
}

table#stationsTable tr:nth-child(odd) {
	background-color: var(--table-odd);
}

a.page {
	font-size: 18pt !important;
	margin-right: 10px;
}

/* ===== Modal Styling ===== */
.modal {
	display: none;
	position: fixed;
	z-index: 80;
	padding-top: 20px;
	left: 0;
	top: 0;
	width: 100%;
	height: 70%;
	overflow: auto;
	background-color: rgba(0, 0, 0, 0.4);
}

.modal-content {
	position: relative;
	background-color: var(--content-bg);
	margin: auto;
	padding: 0;
	border: 1px solid #888;
	width: 80%;
	height: 90%;
	box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19);
	animation-name: animatetop;
	animation-duration: 0.4s;
}

@keyframes animatetop {
	from {
		top: -300px;
		opacity: 0;
	}
	to {
		top: 0;
		opacity: 1;
	}
}

.modal-body {
	padding: 2px 16px;
}

/* ===== Icon Styling ===== */
.icon.icons8:hover {
	box-shadow: 3px 3px 15px 0 rgba(0, 0, 0, 0.24), 3px 3px 3px 0 rgba(0, 0, 0, 0.24);
}

/* ===== Responsive Design ===== */
@media (max-width: 768px) {
	.CONTENT {
		width: 95%;
		padding: 5px;
	}

	.tabs input[type="text"],
	.tabs input[type="password"],
	.tabs select {
		width: 100%;
		margin-bottom: 10px;
	}

	.modal-content {
		width: 95%;
	}
}

/* ===== Selection Color ===== */
::selection {
	background: #ABA06A;
}

::-moz-selection {
	background: #ABA06A;
}
