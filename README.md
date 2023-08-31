# Airwick-ESP8266
Порт проекта модернизации автоматического освежителя Airwick на ESP8266
В основном повторяет предыдущий проект, но с добавлением фишек ESP8266.

## Основные особенности проекта
* Управление освежителем воздуха по WiFI
* Веб интерфейс
* Управление через MQTT
* Включение мотора освежителя кнопкой
* Питание от аккумулятора
* Энергосберегающий режим
* OTA обновление прошивки

## Необходимые компоненты и схема подключения
Для сборки проекта вам необходимы следующие компоненты:
* Автоматический освежитель Airwick
* Фоторезистор
* Mosfet транзистор IRF3205 или аналог
* 2 Резистора на 10 кОм
* 1 резистор на 100 Ом
* Тактовая кнопка без фиксации нажатия
* Wemos D1 Mini или аналог
* Плата зарядки для Li-ION аккумуляторов TP4056 или аналог
* Li-ION аккумулятор 3.7 в 1000 mAh

Схема подключения представлена ниже:
![](/scheme/scheme.png)

## Компиляция и сборка
Все необходимые для сборки библиотеки лежат в папке libraries архива проекта.
Сборка осуществляется в среде Arduino IDE версии 1.8.x и выше.
После загрузки прошивки в плату, необходимо загрузить файловую систему. Для этого вы можете воспользоваться [инструкцией](https://projectalt.ru/publ/arduino_i_esp/rabota_s_fajlovoj_sistemoj_v_addone_esp8266_dlja_ide_arduino/3-1-0-24).
**ВАЖНАЯ ИНФОРМАЦИЯ!!!**. Перед прошивкой перемычку между пинами RST и D0 необходимо выпаять. Иначе загрузка прошивки не запустится.

## Подключение и первичная настройка
При первом запуске устройства автоматически создается точка доступа WiFi **Airwick** с паролем **12345678**. Подключитесь к ней с ноутбука или смартфона и перейдите в браузере по адресу http://192.168.4.1
Откроется окно веб интерфейса:
![](/pictures/main.png)
Перейдите по ссылке Конфигурация. Откроется окно настроек:
![](/pictures/settings.png)
Настройте параметры подключения к WiFi роутеру, нажмите кнопку **Сохранить**. Затем пролистайте страницу настроек вниз и нажмите на кнопку **Перезагрузить устройство**.
![](/pictures/settings2.png)
Далее подключиться к устройству можно, войдя в Сетевое окружение Windows, и выбрав Airwick ESP8266:
![](/pictures/ssdp.png)
Либо ввести присвоенный ему IP адрес в браузере.

## Управление по протоколу MQTT
Настройки протокола MQTT доступны в веб интерфейсе:
![](/pictures/mqtt.png)
Для публикации статусной информации и получения управляющих команд доступны 2 топика. По умолчанию это /status и /command.
В статусном топике публикуется информация о любом изменении состояния устройства: включение, отключение, распыление аэрозоля, изменение параметров и т.п.
Комндный топик служит для получения управляющих команд.
Формат команд: **команда(5 символов) параметр(число)**
Например, для установки интервала срабатывания распылителя 5 минут, вам необходимо отправить в топик **/command** следующее сообщение:
*timer 5*

### Список управляющих команд

| Команда | Параметр (интервал значений) | Описание                                                |
| ------- | ---------------------------- | ------------------------------------------------------- |
| motor   | -                            | Запуск расплителя на 1 секунду                          |
| light   | 1-1024                       | Порог срабатывания датчика света                        |
| pretm   | 1-10                         | Установка интервала предварительного таймера            |
| timer   | 1-60                         | Установка интервала срабатывания распылителя            |
| lowpw   | 0,1                          | Включение и отключение режима низкого энергопотребления |

## Алгоритм работы программы
В целом он практически ничем не отличается от версии для Arduino.
При старте инициализируются глобальные переменные и выполняется подключение к WiFi. Далее запускается веб сервер и выполняется подключение к MQTT брокеру.
В основном цикле запускается три системных таймера:
**Interval** - равен 10 секунд. Отвечает за опрос датчика освещения. Если полученное значение выше установленного порога, то запускается предварительный таймер.
**preTimer** - по умолчанию равен 1 минуте. Служит для фильтрации кратковременных включений света. Если при его срабатывании уровень освещения выше порогового значения, то запускается основной таймер. Если свет был отключен до срабатывания таймера, то он останавливается и запуск основного таймера не производится.
**timerDuration** - по умолчанию равен 1 минуте. Это интервал срабатывания распылителя. При его срабатывании включается мотор распылителя и, если уровень освещения все еще выше порогового значения, таймер перезапускается. Если ниже, то также включается мотор распылителя и таймер останавливается.

### Режим энергосбережения
При включении режима энергосбережения алгоритм несколько меняется. После первичной инициализации и запуска всех служб производится опрос датчика освещения. Если его значение выше порогового значения, то устройство работает как обычно. Запускаются таймеры, проверяются условия срабатывания мотора распылителя, работает веб интерфейс и т.п.
Если значение полученное с датчика освещения ниже порогового, то устройство уходит в глубокий сон (Deep Sleep mode). Отключается процессор, память, wifi и т.д. Работает только системный таймер RTC. В данном режиме устройство потребляет около 0.1 мА. В обычном рабочем режиме потребление около 80 мА. Через 20 секунд срабатывает системный таймер и на пин **D0**, соединенный с пином **RST** подается высокий уровень сигнала и ESP8266 перезагружается. В этот момент наблюдается кратковрменный всплеск энергопотребления до 100 мА, длительностью 1-2 секунды. Снова инициализируются все службы и цикл повторяется. Также при старте опрашивается топик MQTT */command*. Если будет обнаружена команда **lowpw 0**, то режим низкого энергопотребления отключается и устройство работает в обычном режиме.
Настройка интервала нахождения в спящем режиме производится правкой в исходниках программы параметра вызова функции: **ESP.deepSleep(20e6);**. Цифра 20 это и есть число секунд сна.

