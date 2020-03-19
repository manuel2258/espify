module m3_sockel(x, y, z, height) {
    $fn = 35;
    translate([x, y, z+height/2])
    difference() {
        cylinder(r=3.25, h=height, center=true);
        cylinder(r=1.25, h=height, center=true);
    }
}

module m3_hole(x, y, z, height) {
    $fn = 35;
    translate([x, y, z+height/2])
    cylinder(r=1.75, h=height, center=true);
}

module m3_standoff(x, y, z, height) {
    $fn = 35;
    translate([x, y, z+height/2])
    cylinder(r=3, h=height, center=true);
}

module esp_holes(x, y, z, height) {
    translate([x, y, z]) {
        m3_hole(0, 0, 0, height);
        m3_hole(47, 24, 0, height);
        m3_hole(47, 0, 0, height);
        m3_hole(0, 24, 0, height);
    }
}

module esp_standoff(x, y, z, height) {
    translate([x, y, z]) {
        difference() {
            union() {
                m3_standoff(0, 0, 0, height);
                m3_standoff(47, 24, 0, height);
                m3_standoff(47, 0, 0, height);
                m3_standoff(0, 24, 0, height);
            }
            m3_hole(0, 0, 0, height);
            m3_hole(47, 24, 0, height);
            m3_hole(47, 0, 0, height);
            m3_hole(0, 24, 0, height);
        }
    }
}

module body() {
    $fn = 35;
    difference() {
        union() {
            cube(size=[195, 30, 20]);

            translate([0, 30])
            cube(size=[70, 50, 30]);

            esp_standoff(15, 45, 4, 5);
        }
        translate([3, 3, 3])
        cube(size=[189, 24, 17]);
        translate([3, 27, 3])
        cube(size=[64, 50, 27]);

        translate([65, 49, 3])
        cube(size=[5, 15, 10]);

        translate([180, 27, 5])
        cube(size=[5, 5, 5]);

        esp_holes(15, 45, 0, 3);

        m3_hole(15, 7, 0, 3);
        m3_hole(15, 23, 0, 3);

        m3_hole(180, 7, 0, 3);
        m3_hole(180, 23, 0, 3);
    }

    m3_sockel(4.5, 4.5, 0, 20);
    m3_sockel(190.5, 4.5, 0, 20);
    m3_sockel(4.5, 25.5, 0, 20);
    m3_sockel(190.5, 25.5, 0, 20);

    m3_sockel(4.5, 34.5, 0, 30);
    m3_sockel(65.5, 34.5, 0, 30);
    m3_sockel(4.5, 75.5, 0, 30);
    m3_sockel(65.5, 75.5, 0, 30);

    translate([67, 53, 3])
    cube(size=[3, 1, 10]);

    translate([67, 59, 3])
    cube(size=[3, 1, 10]);

    esp_standoff(15, 45, 3, 1);
}

body();