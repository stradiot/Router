# Softvérový smerovač (router)

Projekt IPv4 dvojportového softvérového smerovača v jazyku C++ určeného pre
platformu Linux.

## Použité knižnice a frameworky

Pre spracovávanie, vytváranie a analýzu rámcov využívam voľne dostupnú knižnicu
**libtins**, ktorá je nadstavbou nad knižnicou libpcap a knižnicu boost.

http://libtins.github.io/

Pre zabezpečenie grafického rozhrania využívam C++ framework **Qt vo verzií 5**.

## Funkcionalita

Smerovač umožňuje statické aj dynamické smerovanie. Dynamické smerovanie je
zabezpečené pomocou protokolu RIP verzie 2 a je kompatibilné s Cisco smerovačmi.
Kombatibilita s inými smerovačmi nie je testovaná.

Smerovač zobrazuje RIPv2 databázu v reálnom čase, pričom časovače RIP databázy sú
konfigurovateľné.

Smerovač nepodporuje RIP sumarizáciu, autentifikáciu, ani redistribúciu (mimo
priamo pripojené siete).

RIP proces je možné zapnúť a vypnúť na konkrétnom rozhraní.

Smerovač zobrazuje smerovaciu aj ARP tabuľku v reálnom čase a
podporuje ARP proxy. Časovač pre ARP záznamy je nastaviteľný.

Smerovač podporuje ICMP ping oboma smermi.

Smerovač zobrazuje štatistiky pre každé rozhranie.


![Screenshot of GUI](/home/martin/Desktop/screenshot.png)
