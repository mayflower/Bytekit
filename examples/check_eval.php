<?php
    $fname = $_SERVER['argv'][1];

    $res = bytekit_disassemble_file($fname);

    foreach ($res['functions'] as $key => &$func) {
        $c = &$func['code'];
        for ($i=0; $i<count($c); $i++) {
            if ($c[$i]['mnemonic'] == "EVAL") {
                die("found\n");
            }
        }
    }

    die("not found\n");
?>