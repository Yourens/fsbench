#!/bin/bash

echo sR
for i in {0..8}
do
	echo $i
	sudo ./bench sR $i | grep speed
done

echo sW
for i in {0..8}
do
	echo $i
	sudo ./bench sW $i | grep speed
done

echo rR
for i in {0..8}
do
	echo $i
	sudo ./bench rR $i | grep speed
done

echo rW
for i in {0..8}
do
	echo $i
	sudo ./bench rW $i | grep speed
done
