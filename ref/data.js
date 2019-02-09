var data = {};
data.monitorsCount = 4;
data.tariffsCount = 3;
data.time = '2019-02-06T21:25:26Z';

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
data.current.energy[0] = 22.12;
data.current.hour[0] = 599;
data.current.day[0] = [268180, 0, 61943];
data.current.month[0] = [268180, 0, 61943];
// Monitor 1
data.current.energy[1] = 21.38;
data.current.hour[1] = 598;
data.current.day[1] = [598, 0, 2149];
data.current.month[1] = [598, 0, 2149];
// Monitor 2
data.current.energy[2] = 21.32;
data.current.hour[2] = 599;
data.current.day[2] = [599, 0, 2154];
data.current.month[2] = [599, 0, 2154];
// Monitor 3
data.current.energy[3] = 21.32;
data.current.hour[3] = 600;
data.current.day[3] = [600, 0, 2157];
data.current.month[3] = [600, 0, 2157];

data.hours = [];
// Monitor 0
data.hours[0] = [0, 19693, 0, 0, 0, 42250, 0, 0, 0, 9508, 0, 0, 0, 0, 0, 0, 0, 240492, 17581, 0, 0, 0, 72314, 233497];
// Monitor 1
data.hours[1] = [0, 2149, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 464];
// Monitor 2
data.hours[2] = [0, 2154, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 465];
// Monitor 3
data.hours[3] = [0, 2157, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 466];

data.days = [];
// Monitor 0
data.days[0] = [];
data.days[0][0] = [355927, 372340, 184744, 357165, 261859, 282303, 645616, 331979, 260380, 306294, 478472, 341448, 257389, 330611, 323704, 292330, 292074, 325680, 326660, 119623, 327195, 372388, 319157, 255677, 256090, 54990, 473573, 246926, 289914, 294712, 264663];
data.days[0][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[0][2] = [314927, 316867, 499421, 278542, 486870, 699529, 744312, 718632, 547385, 297270, 506857, 657420, 664943, 669945, 677137, 674747, 533379, 403972, 375266, 534862, 517415, 404305, 404189, 583507, 636863, 746364, 498965, 350826, 343941, 262756, 477325];
// Monitor 1
data.days[1] = [];
data.days[1][0] = [705, 1047, 2673, 4097, 0, 0, 0, 0, 1453, 155856, 0, 0, 0, 2557, 1374, 1381, 0, 2663, 2252, 1830, 2280, 439, 861, 0, 413, 2602, 2039, 4054, 4706, 22, 1097];
data.days[1][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[1][2] = [0, 0, 0, 0, 464, 0, 0, 0, 0, 78187, 452, 0, 0, 0, 5, 0, 0, 432, 451, 0, 0, 910, 910, 0, 0, 0, 585, 692, 1683, 0, 0];
// Monitor 2
data.days[2] = [];
data.days[2][0] = [707, 1048, 2683, 4105, 0, 0, 0, 0, 1454, 159256, 0, 0, 0, 2563, 1375, 1382, 0, 2663, 2252, 1831, 2281, 439, 861, 0, 413, 2602, 2042, 3554, 4206, 18, 1097];
data.days[2][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[2][2] = [0, 0, 0, 0, 465, 0, 0, 0, 0, 79778, 452, 0, 0, 0, 5, 0, 0, 432, 451, 0, 0, 910, 910, 0, 0, 0, 586, 693, 1689, 0, 0];
// Monitor 3
data.days[3] = [];
data.days[3][0] = [708, 1050, 2693, 4113, 0, 0, 0, 0, 1454, 186771, 0, 0, 0, 2567, 1375, 1382, 0, 2665, 2257, 1832, 2283, 439, 861, 0, 414, 2604, 2045, 4072, 4725, 22, 1099];
data.days[3][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.days[3][2] = [0, 0, 0, 0, 466, 0, 0, 0, 0, 93481, 452, 0, 0, 0, 5, 0, 0, 432, 451, 0, 0, 910, 910, 0, 0, 0, 587, 694, 1694, 0, 0];

data.months = [];
// Monitor 0
data.months[0] = [];
data.months[0][0] = [9714044, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5308608, 8125536];
data.months[0][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[0][2] = [16091189, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5573317, 10967036];
// Monitor 1
data.months[1] = [];
data.months[1][0] = [196401, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1549591, 1224689];
data.months[1][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[1][2] = [84723, 0, 0, 0, 0, 0, 0, 0, 0, 0, 830925, 597677];
// Monitor 2
data.months[2] = [];
data.months[2][0] = [198832, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1730414, 1267058];
data.months[2][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[2][2] = [86322, 0, 0, 0, 0, 0, 0, 0, 0, 0, 926522, 619120];
// Monitor 3
data.months[3] = [];
data.months[3][0] = [227431, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2152756, 1527803];
data.months[3][1] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
data.months[3][2] = [100032, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1149540, 754097];
// 1014, 5308