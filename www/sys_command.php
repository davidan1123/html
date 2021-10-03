<?php
	require_once(dirname(__FILE__) . '/config.php');
?>

<?php
$e_user = "pi";

if (file_exists('user.php'))
    include 'user.php';
?>

<?php
if (isset($_GET['cmd']))
	{
	$cmd = $_GET['cmd'];

	if ($cmd === "dav_start")
		{
		$SUDO_CMD = "sudo -u " . $e_user . " " . dav . " > /dev/null 2>&1 &";
		$res = exec($SUDO_CMD);
		}
	else if ($cmd === "dav_stop")
		{
		$fifo = fopen(FIFO_FILE,"w");
		fwrite($fifo, "quit");
		fclose($fifo);
		usleep(500000);
		exec("pgrep dav", $output, $return);
		if ($return == 0)
			exec('killall dav');
		}
	}
?>
