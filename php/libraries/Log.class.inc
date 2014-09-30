<?php
/**
 * Log class file
 *
 * PHP version 5
 *
 * @category Behavioural
 * @package  Main
 * @author   Zia Mohaddes  <zia.mohades@gmail.com>
 * @license  Loris License
 * @link     https://github.com/aces/IBIS
*/

require_once 'PEAR.php';

/**
 * Log Class
 *
 * The class contains the utility functions to create and insert into log file
 *
 * @category Behavioural
 * @package  Main
 * @author   Zia Mohaddes  <zia.mohades@gmail.com>
 * @license  Loris License
 * @link     https://github.com/aces/IBIS
*/
class Log extends PEAR
{

    var $GENERAL_ERROR_DIR;


    /**
     * Adds error message to the file
     *
     * @param String  $message  The content of the message inserted into the log file
     * @param Boolean $use_date It will insert the current date if true
     *
     * @return null
     */
    function addLog($message,$use_date=true)
    {
    }
}
?>
