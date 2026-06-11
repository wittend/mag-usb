// Simple rack panel for Pi Display 2 (small)
//
// (c) 2026 David M. Witten II
//===================================================================
// Constant Parameters
use <rack_plate.scad>
in2mm               = 25.41;  // mm / in

plate_length        = 6.3;    // inches 123 mm
plate_width         = 160.0;  // mm
plate_thickness     = 3.0;    // mm

left_notch_width    = 4.0;    // mm
left_notch_ht       = 50.0;   // mm

mnt_hole_width      = 7.5;    // mm
mnt_hole_len        = 11.0;   // mm

//===================================================================
// Modifiable Parameters


//===================================================================
// Main()
module cutout()
{
    display_cutout = cube([127, 73, 16]);
}

//===================================================================
// main()
module main()
{
    difference()
    {
        rack_plate(u_count = 2, thickness = plate_thickness, width_inches = plate_length);
        translate([(140 - 123), 6, -8])
        {
            cube([126, 75, 16]);
            //#cutout();
        }
        translate([(140 - 127), (left_notch_ht/3)+2 , -8])
        {
            cube([left_notch_width+ 0.5, left_notch_ht, 16]);
            //#cutout();
        }
    }
}

main();