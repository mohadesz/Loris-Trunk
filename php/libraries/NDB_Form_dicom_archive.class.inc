<?php
/**
 * form_dicom_archive class file
 *
 * PHP version 5
 *
 * @category   Behavioural
 * @package    Main
 * @subpackage Imaging
 * @author     Olga Tsibulevskaya <olgatsib@gmail.com>
 * @license    Loris License
 * @link       https://github.com/aces/Loris-Trunk
*/

require_once 'NDB_Form.class.inc';

/**
 * NDB_Form_Dicom_Archive Class
 *
 * This class is dicom archive Form
 *
 * @category   Behavioural
 * @package    Main
 * @subpackage Imaging
 * @author     Olga Tsibulevskaya <olgatsib@gmail.com>
 * @license    Loris License
 * @link       https://github.com/aces/Loris-Trunk
*/

class NDB_Form_Dicom_Archive extends NDB_Form
{
    /**
    * Sets up main parameters
    *
    * @return NULL
    */
    function viewDetails()
    {
        $this->DB = Database::singleton();
        if (Utility::isErrorX($DB)) {
            print "Could not connect to database: ".$DB->getMessage()."<br>\n";
            die();
        }

        if (!empty($_REQUEST['tarchiveID'])) {
            $tarchiveID = $_REQUEST['tarchiveID'];
            $this->tpl_data['archive'] = $this->_getTarchiveData(
                $tarchiveID, 'tarchive'
            );
            $this->tpl_data['archive_series'] = $this->_getTarchiveData(
                $tarchiveID, 'tarchive_series', 'TarchiveSeriesID'
            );
            $this->tpl_data['archive_files'] = $this->_getTarchiveData(
                $tarchiveID, 'tarchive_files', 'TarchiveFileID'
            );

            $this->_validateNamesIDs();
        }
    }
    /**
    * Retrieves data for the template from the database
    * 
    * @param int    $tarchiveID tarchiveID
    * @param string $table      name of the table to retrieve the data from
    * @param string $order      column by which to order the query
    * 
    * @return array $array contains all data
    */
    function _getTarchiveData($tarchiveID, $table, $order) 
    {
        if ($order) {    	     				   
            $query = "SELECT * FROM $table WHERE TarchiveID ='$tarchiveID'
            ORDER BY $order";
            $array = $this->DB->pselect($query, array());
        } else {
            $query = "SELECT * FROM $table WHERE TarchiveID ='$tarchiveID'";
            $array = $this->DB->pselectRow($query, array());
        }
        
        if (Utility::isErrorX($array)) { 
            print $array->getMessage()."<br>\n";
            return null;
        } else {
            return $array;
        }
    }
    /**
    * Validates PatientName and PatientID,
    * sets INVALID-HIDDEN if not validated.
    *
    * @return null
    */
    function _validateNamesIds() 
    {
        $config =& NDB_Config::singleton();
        $dicomArchiveSettings = $config->getSetting('dicom_archive');
        
        if ((preg_match(
            $dicomArchiveSettings['patientNameRegex'], 
            $this->tpl_data['archive']['PatientName']
        )) || (preg_match(
            $dicomArchiveSettings['LegoPhantomRegex'],
            $this->tpl_data['archive']['PatientName']
        )) || (preg_match(
            $dicomArchiveSettings['LivingPhantomRegex'],
            $this->tpl_data['archive']['PatientName']
        ))) {
            $this->tpl_data['archive']['patientNameValid'] = 1;
        } else {
            $this->tpl_data['archive']['patientNameValid'] = 0;
            $this->tpl_data['archive']['PatientName'] = "INVALID - HIDDEN";
        }
        
        if (preg_match(
            $dicomArchiveSettings['patientIDRegex'],
            $this->tpl_data['archive']['PatientID']
        )) {
            $this->tpl_data['archive']['patientIDValid'] = 1;
        } else {
            $this->tpl_data['archive']['patientIDValid'] = 0;
            $this->tpl_data['archive']['PatientID'] = "INVALID - HIDDEN";
        }
    }
}
?>