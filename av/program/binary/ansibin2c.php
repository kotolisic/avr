<?php 

// php ansibin2c.php > ../include/ansi3.h

$k  = 0;
$im = imagecreatefrompng("font4x8_beta.png");
$as = [];

for ($y = 0; $y < 24; $y += 8) {
    for ($x = 0; $x < 128; $x += 8) {

        for ($i = 0; $i < 8; $i++) {

            $byte = 0;
            for ($j = 0; $j < 8; $j++) {

                $pix = imagecolorat($im, $x + $j, $y + $i);
                if ($pix == 0) {
                    $byte |= (1 << (7 - $j));
                }
            }
            $as[$k++] = $byte;
        }
    }
}

echo "static const char ansi3[48][8] = {\n";
for ($i = 32; $i < 32 + 48; $i++) {

    echo "    /* ".str_pad(dechex(($i-32)*2 + 32), 2, '0', STR_PAD_LEFT)." */ { ";
    for ($j = 0; $j < 8; $j++) {
        echo '0x'.str_pad(dechex($as[8*($i-32)+$j]), 2, '0', STR_PAD_LEFT) . ($j < 7 ? ', ' : ' ');
    }
    echo "},\n";
}
echo "};";
