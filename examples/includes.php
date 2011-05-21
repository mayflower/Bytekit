<?php
    define("MODULE", "module1");
    define("BASE_PATH", "/directory/");
    
    include "/konstante.php";
    include BASE_PATH . MODULE . ".php";
    include $x . ".php";
    $x = MODULE . "1.";
    $y = $x;
    include $y . ".php";
?>