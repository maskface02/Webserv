<?php
session_start();

if (!isset($_SESSION['counter'])) {
    $_SESSION['counter'] = 0;
}
$_SESSION['counter']++;

$counter = $_SESSION['counter'];
$session_id = session_id();

echo "<html><head><title>PHP Session Test</title>";
echo "<style>body{font-family:monospace;margin:20px}.info{background:#E1BEE7;padding:15px;border-radius:5px;margin:10px 0}</style>";
echo "</head><body>";
echo "<h1>PHP Session Test</h1>";
echo "<div class='info'>";
echo "<p><b>Session ID:</b> " . htmlspecialchars($session_id) . "</p>";
echo "<p><b>Visit Counter:</b> " . $counter . "</p>";
echo "<p><b>Cookie:</b> " . (isset($_COOKIE['PHPSESSID']) ? htmlspecialchars($_COOKIE['PHPSESSID']) : 'Not set') . "</p>";
echo "</div>";
echo "<p>Refresh the page to increment the counter.</p>";
echo "<p><a href='/php/session_test.php'>Refresh</a></p>";
echo "</body></html>";
?>
