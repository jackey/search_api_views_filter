<?php 

class search_api_views_filter_in_operator extends views_handler_filter {
 function options_form (&$form, &$form_state) {
   parent::options_form();
    $options = array();
    foreach (search_api_index_load_multiple(FALSE) as $index) {
      $options[$index->machine_name] = 'Search API Index :' . $index->name;
    }
    
    $form['value'] = array(
     '#type' => 'select',
     '#options' => $options,
     '#multiple' => FALSE,
    );
 }
 
 function can_expose() {
	return FALSE;
 }
 
 function query() {
   $entity_id = array();
   $query = search_api_index_load($this->options['value'])->query(array('parse mode' => 'terms'));
   	 if ($query->getOption('search id') == get_class($query)) {
   	 	$query->setOption('search id', 'search_api_views_filter:' . $this->view->name . ':' . $this->view->current_display);
   	 }
     $results = $query->execute();
     foreach ($results['results'] as $id => $result) {
       if (!empty($result['entity'])) {
       	 $entity_id[] = $result['entity']->{$result['entity']->idKey};
       }
       $entity_id[] = $id;
     }
   if (!empty($entity_id)) {
   	$this->ensure_my_table();
    $this->query->add_where($this->options['group'], "$this->table_alias.$this->real_field", $entity_id, 'in');
   }
 }
}