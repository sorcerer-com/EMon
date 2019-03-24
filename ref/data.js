var data = {};
data.monitorsCount = 4;
data.tariffsCount = 3;
data.time = '2019-03-23T23:31:26Z';
data.startTime = 1552823934;

data.settings = {};
data.settings.timeZone = 2;
data.settings.tariffStartHours = [];
data.settings.tariffPrices = [];
data.settings.tariffStartHours[0] = 7;
data.settings.tariffPrices[0] = 0.17732;
data.settings.tariffStartHours[1] = 23;
data.settings.tariffPrices[1] = 0.10223;
data.settings.tariffStartHours[2] = 23;
data.settings.tariffPrices[2] = 0.10223;
data.settings.billDay = 6;
data.settings.currencySymbols = 'lv.';
data.settings.monitorsNames = [];
data.settings.monitorsNames[0] = 'Lampi, Kuhnia, Kotle';
data.settings.monitorsNames[1] = 'Furna, Hol, Kotle';
data.settings.monitorsNames[2] = 'Boiler, Spalnia, Detska, Kotle';
data.settings.monitorsNames[3] = '';
data.settings.coefficient = 1.00;
data.settings.wifi_ssid = 'm68';
data.settings.wifi_passphrase = 'bekonche';
data.settings.wifi_ip = '192.168.0.105';
data.settings.wifi_gateway = '192.168.0.1';
data.settings.wifi_subnet = '255.255.255.0';
data.settings.wifi_dns = '192.168.0.1';

data.current = {};
data.current.energy = [];
data.current.hour = [];
data.current.day = [];
data.current.month = [];
// Monitor 0
data.current.energy[0] = 586.81;
data.current.hour[0] = 24904;
data.current.day[0] = [394494, 0, 56531];
data.current.month[0] = [6630767, 0, 2760762];
// Monitor 1
data.current.energy[1] = 103.70;
data.current.hour[1] = 5051;
data.current.day[1] = [77635, 0, 5695];
data.current.month[1] = [3479174, 0, 457734];
// Monitor 2
data.current.energy[2] = 2485.12;
data.current.hour[2] = 127088;
data.current.day[2] = [319579, 0, 171665];
data.current.month[2] = [4996207, 0, 5462885];
// Monitor 3
data.current.energy[3] = 21.80;
data.current.hour[3] = 0;
data.current.day[3] = [0, 0, 0];
data.current.month[3] = [75874, 0, 26640];

data.hours = [];
// Monitor 0
data.hours[0] = [4330, 4382, 5058, 3970, 5413, 4333, 4141, 5452, 4008, 7029, 3315, 5654, 3940, 3911, 6195, 80949, 28366, 14871, 13000, 81197, 50760, 41473, 44374, 6964];
// Monitor 1
data.hours[1] = [262, 81, 50, 49, 32, 151, 19, 0, 17, 11680, 1395, 34, 133, 49, 48, 938, 5091, 1409, 238, 18663, 18803, 9512, 9625, 2192];
// Monitor 2
data.hours[2] = [0, 0, 0, 41026, 654, 1689, 1208, 0, 0, 0, 9773, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 66287, 243519, 98119];
// Monitor 3
data.hours[3] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

data.days = [];
// Monitor 0
data.days[0] = [];
data.days[0][0] = [0, 0, 0, 0, 0, 10714, 219911, 163537, 223564, 392523, 411640, 1028337, 671070, 792562, 656062, 406338, 211257, 278013, 149138, 241294, 237925, 142388, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[0][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[0][2] = [0, 0, 0, 0, 0, 22123, 63190, 68883, 76384, 65673, 99708, 311407, 383497, 393831, 449693, 389360, 80884, 73300, 78416, 50994, 56908, 39980, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 1
data.days[1] = [];
data.days[1][0] = [0, 0, 0, 0, 0, 16308, 240530, 215844, 251696, 274115, 175259, 406206, 235805, 188350, 151623, 120345, 221432, 135894, 329609, 139254, 243797, 55472, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[1][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[1][2] = [0, 0, 0, 0, 0, 8212, 53852, 54336, 51814, 59271, 51942, 51356, 53344, 6414, 6378, 7346, 14599, 7198, 6989, 7590, 9206, 2192, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 2
data.days[2] = [];
data.days[2][0] = [0, 0, 0, 0, 0, 148924, 301463, 40693, 319688, 274110, 362086, 921889, 226434, 471678, 4775, 391709, 201520, 171333, 241498, 207178, 382772, 8878, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[2][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[2][2] = [0, 0, 0, 0, 0, 230670, 358091, 384053, 309375, 278066, 372569, 303934, 486530, 400064, 356256, 244957, 181114, 303609, 290009, 208246, 265849, 317828, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 3
data.days[3] = [];
data.days[3][0] = [0, 0, 0, 0, 0, 3431, 8980, 11016, 11743, 8459, 12224, 2138, 13574, 0, 0, 1946, 2363, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[3][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[3][2] = [0, 0, 0, 0, 0, 138, 3820, 3516, 3897, 4867, 4968, 2852, 2582, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

data.months = [];
// Monitor 0
data.months[0] = [];
data.months[0][0] = [0, 10714, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[0][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[0][2] = [0, 22123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 1
data.months[1] = [];
data.months[1][0] = [0, 16308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[1][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[1][2] = [0, 8212, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 2
data.months[2] = [];
data.months[2][0] = [0, 148924, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[2][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[2][2] = [0, 230670, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// Monitor 3
data.months[3] = [];
data.months[3][0] = [0, 3431, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[3][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[3][2] = [0, 138, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
// 131, 5334