#!/usr/bin/env python
# -*- coding: utf-8 -*-
'''
Program można uruchamiać jak skrypt w konsoli:
	./LEDappPython.py
Wcześniej należy mu nadać prawa do wykonywania:
	chmod +x LEDappPython.py
Niezbęta może być też instalacja pakietu tkinter:
	python-tk
'''
from tkinter import *
from tkinter import messagebox
import serial
import time
__author__ = "Igor Sołdrzyński"
__license__ = "GPL"
__version__ = "0.1"
__maintainer__ = "Igor Sołdrzyński"
__email__ = "igor.soldrzynski@gmail.com"
__status__ = "Beta"


### dopisać wyświetlanie akrualnych wartości wielowątkowo #########################

#obiekty globalne
okno = Tk()
mocWhite = Entry(okno)
mocBlue = Entry(okno)
mocUv = Entry(okno)
gStartWhite = Entry(okno)
gStartBlue = Entry(okno)
gStartUv = Entry(okno)
gStopWhite = Entry(okno)
gStopBlue = Entry(okno)
gStopUv = Entry(okno)
#lista pól
pola=[]
pola.append(mocWhite)
pola.append(mocBlue)
pola.append(mocUv)
pola.append(gStartWhite)
pola.append(gStartBlue)
pola.append(gStartUv)
pola.append(gStopWhite)
pola.append(gStopBlue)
pola.append(gStopUv)


#funkcja łączącza się z arduino
def polacz(port):
	global arduino
	try:
		arduino = serial.Serial(port,9600,timeout=0)
		time.sleep(2)
	except:
		messagebox.showerror("Błąd połączenia z Arduino!","Brak połączenia z Arduino. Sprawdż podany port...")
	else:
		messagebox.showinfo("Połączono", "Połączono z Arduino.")

#pobranie ustawień z arduino
def pobierzUstawienia():
	#format z arduino: 0.82;0.85;0.90;9.50;8.80;8.50;20.5;21.2;21.5;
	arduino.write("pobierz")
	time.sleep(1.5)
	tekst=arduino.readline() #tekst pobrany z arduino
	time.sleep(0.2)

	j=0
	for i in range(len(pola)):
		pola[i].delete(0,END) #czyszczenie pola przed insertem
		pola[i].insert(0,tekst[j:j+4])
		j+=5


#wysłanie ustawień do arduino
def wyslijUstawienia():
	ustawienia = "wyslij"
	#do ciągu ustawienia doklej wartość wszystkich pól i wyślij
	for i in range(len(pola)):
		if(len(pola[i].get())!=4):
			messagebox.showerror("Błędna wartość!", "Podałeś błędną wartość w jednym z pól. Wartości muszą mieć 4 znaki (z przecinkiem).")
			raise(ValueError)
		ustawienia = ustawienia + pola[i].get() + ";"
	arduino.write(ustawienia)
	print(ustawienia)
	time.sleep(1.5)


#wysłanie prostego polecenia (zapisz lub szczyt)
def polecenieArduino(polecenie):
	arduino.write(polecenie)
	time.sleep(1.5)


#funckja rysująca okno główne programu
def rysujOkno():
	#nowe okno tkinter
	okno.title("LEDappPython - zmiana ustawień lampy LED")
	wiersz=0


	#pole łączenia:
	Label(okno, text="Port Arduino:").grid(row=wiersz, column=1, sticky=E)
	polePort = Entry(okno)
	polePort.grid(row=wiersz, column=2)
	polePort.insert(0,"/dev/ttyUSB0")
	Button(okno, text='Połącz', command=lambda: polacz(polePort.get())).grid(row=wiersz, column=3, sticky=W)
	wiersz+=1


	#stworzenie etykiet (nagłówków) kolumn:
	etykietyKolumn=["WHITE", "BLUE", "UV"]
	tlaEtykietKolumn=["floral white", "dodger blue", "orchid"]
	kolumna=1
	for i in etykietyKolumn:
		etykieta = Label(okno, text=i)
		etykieta.grid(row=wiersz, column=kolumna, sticky=W+E+N+S)
		etykieta.config(font=(18), bg=tlaEtykietKolumn[kolumna-1])
		kolumna+=1
	wiersz+=1


	#stworzenie etykiet (nagłówków) wierszy:
	etykietyKolumn=["Moc", "Godzina start", "Godzina stop"]
	kolumna=0
	for i in etykietyKolumn:
		etykieta = Label(okno, text=i)
		etykieta.grid(row=wiersz, column=kolumna, sticky=E)
		etykieta.config(font=(14), justify="right")
		wiersz+=1

	#stworzenie pól moc:
	mocWhite.grid(row=wiersz-3, column=1)
	mocBlue.grid(row=wiersz-3, column=2)
	mocUv.grid(row=wiersz-3, column=3)

	#stworzenie godz. start:
	gStartWhite.grid(row=wiersz-2, column=1)
	gStartBlue.grid(row=wiersz-2, column=2)
	gStartUv.grid(row=wiersz-2, column=3)

	#stworzenie godz. stop:
	gStopWhite.grid(row=wiersz-1, column=1)
	gStopBlue.grid(row=wiersz-1, column=2)
	gStopUv.grid(row=wiersz-1, column=3)

	
	#przyciski:
	Button(okno, text='Godzina szczytu na 5 min.', command=lambda: polecenieArduino("szczyt")).grid(row=wiersz, column=0)
	Button(okno, text='Pobierz aktualne', command=lambda: pobierzUstawienia()).grid(row=wiersz, column=1)
	Button(okno, text='Wyślij zmienione', command=lambda: wyslijUstawienia()).grid(row=wiersz, column=2)
	Button(okno, text='Zapisz w EEPROM', command=lambda: polecenieArduino("zapisz")).grid(row=wiersz, column=3)
	wiersz+=1


	#opis funkcji:
	opis="""
Moc: 
	Współczynnik mocy kanału. Jego moc w godzinie szczytu.

Godzina start/stop: 
	Godziny włączenia/wyłączenia kanału.

Godzina szczytu na 5 min.: 
	Ustawia maksymalne (wg. pól Moc) moce kanałów na 5 minut.

Pobierz aktualne: 
	Pobiera aktualne ustawienia z Arduino.

Wyślij zmienione: 
	Wysyła ustawienia do Arduino.

Zapisz w EEPROM: 
	Zapisuje ustawienia w pamięci EEPROM Arduino na wypadek utraty zasilania.
	"""
	Label(okno, text=opis, fg="gray15", justify="left", font=("Sans", 11)).grid(row=wiersz, column=0, columnspan=4)
	wiersz+=1
	

	#podpis:
	Label(okno, text="LEDappPython v0.1\nIgor Sołdrzyński", font=("Sans", 10), fg="dim gray", justify="right").grid(row=wiersz, column=3, sticky=E)

	
	okno.mainloop()	


def main():
	rysujOkno()


#polacz()
main()
