<?php
header('Content-Type: text/html; charset=utf-8');
?>
<!DOCTYPE html>
<html>
<body>
    <h1>Hello from PHP</h1>
    <p>Server: <?= htmlspecialchars($_SERVER['SERVER_SOFTWARE'] ?? '') ?></p>
    <p>Method: <?= htmlspecialchars($_SERVER['REQUEST_METHOD'] ?? '') ?></p>
    <p>Remote: <?= htmlspecialchars($_SERVER['REMOTE_ADDR'] ?? '') ?></p>
</body>
</html>
