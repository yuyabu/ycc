#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./ycc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 0 0
try 42 42
try 21 '5+20-4'
try 41 ' 12 + 34 - 5'
try 47 '5+6*7'
try 15 '5*(9-6)'
try 4 '(3+5)/2'
try 3 '-(-3)'
try 8 '-(3-11)'
try 15 '-(-3*+5)'
try 1 ' 4==4'
try 0 '3==-3'
try 1 '4!=-3'
try 0 '-3!=-3'
try 1 '3<4'
try 0 '-3<-4'
try 1 '-4 <= 100'
try 1 '4<=4'
try 1 '8<=11'
try 0 '-3<=-4'
echo OK
