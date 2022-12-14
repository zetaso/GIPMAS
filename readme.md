## Los siguientes pasos se hacen en Eclipse Runtime.

# Requisito: DSL a MR extendido

Para la generación de código SQL desde un modelo
de Empresa de entrega de encomiendas, se requiere
primero pasar a Modelo Relacional extendido, es
decir, conteniendo además las tuplas del modelo
de origen.

Para ello, se requiere ejecutar el proyecto QVT
para transformar primero el modelo de interés a
Modelo Relacional.

# MR extendido a SQL

Para la ejecución de la transformación modelo a
texto, se debe clickear en:

Run
Run Configurations
click derecho en Acceleo Application
New Configuration

A continuación se deben llenar los campos:

Project:		ProyectoAText
Main class:		ProyectoAText.main.Generate
Model:			/salidaProyecto2022/Proyecto2Relational.RDBMS2
Target:			[directorio de salida a elección]

Apply
Run