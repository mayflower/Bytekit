<?php
    $fname = $_SERVER['argv'][1];
    $fname = realpath($fname);

    if ($fname[strlen($fname)-1] != '/') {
        $fname .= '/';
    }

    $iterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($fname));

    $list = array();
    foreach ($iterator as $file) {    
        if ($file->isFile()) {
            $relName = $file->getPathName();
            if (preg_match('#^.*\.php$#', $relName, $res)) {
            
                $found = array();
            
                $res = bytekit_disassemble_file($relName);
                foreach ($res['functions'] as $key => &$func) {
                    $c = &$func['code'];
                    for ($i=0; $i<count($c); $i++) {
                        if ($c[$i]['mnemonic'] == "EVAL") {
                            $opline = $c[$i]['opline'];
                            $lineno = $func['raw']['opcodes'][$opline]['lineno'];
                            $found[] = $lineno;
                        }
                    }
                }
            
                if (count($found) == 0) continue;
            
                sort($found);
            
                $fileData = file($relName);
            
                for ($i=0; $i<count($found); $i++) {
                    echo $relName,"(",$found[$i],"): ",trim($fileData[$found[$i]-1]),"\n";
                }
            
            }
        }
    }
?>