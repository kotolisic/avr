<?php

$rows = [];
$file = file_get_contents($argv[1]);

for ($cursor = 0; $cursor < strlen($file); $cursor += 2) {

    $ir = ord($file[$cursor]) + 256*ord($file[$cursor+1]);
    $rows[] = "  " . str_pad( dechex($cursor >> 1), 4, '0', STR_PAD_LEFT) . " :  " . str_pad( dechex($ir), 4, '0', STR_PAD_LEFT) . ";"; 
}

$rows[] = "  [" . str_pad( dechex(strlen($file) >> 1), 4, '0', STR_PAD_LEFT) . " .. ffff]: 00;";

?>
WIDTH=16;
DEPTH=65536;

ADDRESS_RADIX=HEX;
DATA_RADIX=HEX;
CONTENT BEGIN
<? echo join("\n", $rows) . "\n"; ?>
END;

