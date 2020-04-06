# SO-TPO1
##Instrucciones de compilacion y ejecucion
Para compilar el se debe simplemente correr el siguiente comando en el working directory:
make
Una vez compilado el código, se deberían generar tres archivos binarios: master.o, slave.o y view.o. 
El archivo master.o y view.o seran los archivos que se podrán ejecutar. Para ejecutar el máster simplemente hacemos:
```./master.o [Archivos a Ejecutar]```
dónde [Archivos a ejecutar] son todo los archivos que se desee que procesen los slaves. Al inicializar el máster se retorna un número por stdout, el cual se debe enviar al programa view, ya se mediante un parámetro o utilizando un pipe de la siguiente forma:

```Bash
./view [Resultado de master]
.master.o [Archivos a ejecutar] | ./view
```

