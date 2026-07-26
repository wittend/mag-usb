// Simple rack panel for Pi Display 2 (small)
//
// (c) 2026 David M. Witten II
//===================================================================
// Constant Parameters
use <rack_plate.scad>
in2mm               = 25.41;  // mm / in

plate_length        = 16.0;   // mm
plate_width         = 160.0;  // mm
plate_thickness     = 2.0;    // mm

mnt_hole_width      = 7.5;    // mm
mnt_hole_len        = 11.0;   // mm

//===================================================================
// Modifiable Parameters


//===================================================================
// Main()
module cutout()
{
    display_cutout = cube([120, 75, 16]);
}

//===================================================================
// main()
module main()
{
    difference()
    {
        rack_plate(u_count = 1, thickness = plate_thickness, width_inches = (plate_length/(in2mm/10)));
        translate([(140 - 120.75), 5.5, -8])
        {
            cube([121.5, 75.75, 16]);
        }
    }
}

main();