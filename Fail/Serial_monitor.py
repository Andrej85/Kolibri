VERSION = "v0.09"
from PyQt5 import QtGui, QtCore
from PyQt5.QtWidgets import QTextEdit, QWidget, QApplication, QVBoxLayout
 
try:
    import Queue
except:
    import queue as Queue
import sys, time, serial
 
WIN_WIDTH, WIN_HEIGHT = 684, 400    # Размер окна
SER_TIMEOUT = 0.1                   # Тайм-аут для последовательного Rx
RETURN_CHAR = "\n"                  # Символ, который будет отправлен при нажатии клавиши Enter
PASTE_CHAR  = "\x16"                # Код Ctrl для вставки из буфера обмена
baudrate    = 115200                # Скорость передачи по умолчанию
portname    = "COM20"               # Имя порта по умолчанию
hexmode     = False                 # Флаг для включения шестнадцатеричного отображения
 
# Преобразование строки в байты
def str_bytes(s):
    return s.encode('latin-1')
     
# Преобразовать байты в строку
def bytes_str(d):
    return d if type(d) is str else "".join([chr(b) for b in d])
     
# Возврат шестнадцатеричных значений данных
def hexdump(data):
    return " ".join(["%02X" % ord(b) for b in data])
 
# Возвращает строку с замененными символами в старшем разряде шестнадцатеричными значениями.
def textdump(data):
    return "".join(["[%02X]" % ord(b) if b>'\x7e' else b for b in data])
     
# Отображение входящих последовательных данных
def display(s):
    if not hexmode:
        sys.stdout.write(textdump(str(s)))
    else:
        sys.stdout.write(hexdump(s) + ' ')
 
# Пользовательское текстовое поле, улавливание нажатий клавиш
class MyTextBox(QTextEdit):
    def __init__(self, *args): 
        QTextEdit.__init__(self, *args)
         
    def keyPressEvent(self, event):     # Отправить нажатие клавиши родительскому обработчику
        self.parent().keypress_handler(event)
             
#Главный виджет          
class MyWidget(QWidget):
    text_update = QtCore.pyqtSignal(str)
         
    def __init__(self, *args): 
        QWidget.__init__(self, *args)
        self.textbox = MyTextBox()              # Создать собственное текстовое поле
        font = QtGui.QFont()
        font.setFamily("Courier New")           # Моноширинный шрифт
        font.setPointSize(10)
        self.textbox.setFont(font)
        layout = QVBoxLayout()
        layout.addWidget(self.textbox)
        self.setLayout(layout)
        self.resize(WIN_WIDTH, WIN_HEIGHT)              # Установить размер окна
        self.text_update.connect(self.append_text)      # Подключить текстовое обновление к обработчику
        sys.stdout = self                               # Перенаправить sys.stdout на себя
        self.serth = SerialThread(portname, baudrate)   # Начать последовательный поток
        self.serth.start()
         
    def write(self, text):                      # Обработать sys.stdout.write: обновить отображение
        self.text_update.emit(text)             # Отправить сигнал для синхронизации вызова с основным потоком
         
    def flush(self):                            # Обработка sys.stdout.flush: ничего не делать
        pass
 
    def append_text(self, text):                # Обработчик обновления текстового дисплея
        cur = self.textbox.textCursor()
        cur.movePosition(QtGui.QTextCursor.End) # Переместите курсор в конец текста
        s = str(text)
        while s:
            head,sep,s = s.partition("\n")      # Разделить линию на НЧ
            cur.insertText(head)                # Вставить текст под курсором
            if sep:                             # Новая строка, если LF
                cur.insertBlock()
        self.textbox.setTextCursor(cur)         # Обновить видимый курсор
 
    def keypress_handler(self, event):          # Обработка нажатия клавиш из текстового поля
        k = event.key()
        s = RETURN_CHAR if k==QtCore.Qt.Key_Return else event.text()
        if len(s)>0 and s[0]==PASTE_CHAR:       # Обнаружить пасту ctrl-V
            cb = QApplication.clipboard() 
            self.serth.ser_out(cb.text())       # Отправить строку вставки в драйвер последовательного порта
        else:
            self.serth.ser_out(s)               # ..Или отправить нажатие клавиши
     
    def closeEvent(self, event):                # Закрытие окна
        self.serth.running = False              #Подождите, пока не завершится последовательный поток
        self.serth.wait()
         
# Поток для обработки входящих и исходящих последовательных данных
class SerialThread(QtCore.QThread):
    def __init__(self, portname, baudrate): # Инициализировать с подробностями последовательного порта
        QtCore.QThread.__init__(self)
        self.portname, self.baudrate = portname, baudrate
        self.txq = Queue.Queue()
        self.running = True
 
    def ser_out(self, s):                   # Записать исходящие данные в последовательный порт, если он открыт
        self.txq.put(s)                     # ..Использование очереди для синхронизации с потоком читателя
         
    def ser_in(self, s):                    # Записывать входящие последовательные данные на экран
        display(s)
         
    def run(self):                          # Запустить поток последовательного считывателя
        print("Открытие %s на %u бод %s" % (self.portname, self.baudrate,
              "(hex display)" if hexmode else ""))
        try:
            self.ser = serial.Serial(self.portname, self.baudrate, timeout=SER_TIMEOUT)
            time.sleep(SER_TIMEOUT*1.2)
            self.ser.flushInput()
        except:
            self.ser = None
        if not self.ser:
            print("Не могу открыть порт")
            self.running = False
        while self.running:
            s = self.ser.read(self.ser.in_waiting or 1)
            if s:                                       # Получить данные из последовательного порта
                self.ser_in(bytes_str(s))               # ..И преобразовать в строку
            if not self.txq.empty():
                txd = str(self.txq.get())               # Если данные Tx в очереди, запись в последовательный порт
                self.ser.write(str_bytes(txd))
        if self.ser:                                    # Закройте последовательный порт, когда поток завершится
            self.ser.close()
            self.ser = None
             
if __name__ == "__main__":
    app = QApplication(sys.argv) 
    opt = err = None
    for arg in sys.argv[1:]:                # Process command-line options
        if len(arg)==2 and arg[0]=="-":
            opt = arg.lower()
            if opt == '-x':                 # -X: отображать входящие данные в шестнадцатеричном формате
                hexmode = True
                opt = None
        else:
            if opt == '-b':                 # -B число: скорость передачи, например '9600'
                try:
                    baudrate = int(arg)
                except:
                    err = "Неверная скорость передачи '%s'" % arg
            elif opt == '-c':               # -C порт: имя последовательного порта, например 'COM1'
                portname = arg
    if err:
        print(err)
        sys.exit(1)
    w = MyWidget()
    w.setWindowTitle('Последовательный терминал PyQT ' + VERSION)
    w.show() 
    sys.exit(app.exec_())
         
# EOF