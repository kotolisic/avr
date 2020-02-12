<?php 

$bin = file_get_contents($argv[1]);
$size = filesize($argv[1]);
for ($i = 0; $i < $size; $i += 2) {

    $a = ord($bin[$i]);
    $b = ord($bin[$i+1]);
    $c = (256*$b + $a);

    echo str_pad(dechex($c), 4, '0', STR_PAD_LEFT) . "\n";
}