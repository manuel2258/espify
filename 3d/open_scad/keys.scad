// To be used with KeyV2 https://github.com/rsheldiii/KeyV2

include <./includes.scad>

legend("«", size=9) 
cherry()
key();

translate([25, 0, 0])
legend("||", size=6) 
cherry()
key();

translate([50, 0, 0])
legend("»", size=9) 
cherry()
key();

translate([75, 0, 0])
legend("̶", size=8) 
cherry()
key();

translate([100, 0, 0])
legend("^", size=9) 
cherry()
key();

translate([125, 0, 0])
legend("+", size=9) 
cherry()
key();
