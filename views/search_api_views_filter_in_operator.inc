<?php 

class search_api_views_filter_in_operator extends views_handler_filter {
 function options_form (&$form, &$form_state) {
   parent::options_form();
   $options = array();
   $views = views_get_all_views();
   foreach ($views as $key => $view) {
     if (substr($view->base_table, 0, 17) == 'search_api_index_') {
     	foreach ($view->display  as $display) {
     	  if (in_array($display->display_plugin, array('page', 'block'))) {
     	  	$options[$view->name . ':' . $display->id] = $view->human_name .':' . $display->id; 
     	  }
     	}
     }
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
   $option = explode(':', $this->options['value']);
   $view = views_get_view($option[0]);
   $view->set_display($option[$option[1]]);
   if (substr($view->base_table, 0, 17) == 'search_api_index_') {
   	 $machine_name = substr($view->base_table, 17);
   	 $query = search_api_index_load($machine_name)->query(array('parse mode' => 'terms'));
   	 if ($query->getOption('search id') == get_class($query)) {
   	 	$query->setOption('search id', 'search_api_views:' . $view->name . ':' . $view->current_display);
   	 }
     $results = $query->execute();
     foreach ($results['results'] as $id => $result) {
       if (!empty($result['entity'])) {
       	 $entity_id[] = $result['entity']->{$result['entity']->idKey};
       }
       $entity_id[] = $id;
     }
   }
   if (!empty($entity_id)) {
   	$this->ensure_my_table();
    $this->query->add_where($this->options['group'], "$this->table_alias.$this->real_field", $entity_id, 'in');
   }
 }
}