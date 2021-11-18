<?php
// phpinfo();
// header("HTTP/1.1 404 Not Found");
header('Content-type: image/png', true);
// exit;
//
// // phpinfo();
$entityBody = file_get_contents('php://input');
echo($entityBody);
// echo(1);
