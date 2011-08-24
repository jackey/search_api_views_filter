<?php 

class search_api_views_filter_in_operator extends views_handler_filter {
 
 /**
  * Extend.
  * @see views_handler_filter::option_definition()
  */
 function option_definition() {
 	$options = parent::option_definition();
 	$options['mode'] = array('default' => 'keys');
 	$options['fields'] = array('default' => array());

 	return $options;
 }
 
 /**
  * Value form.
  * @see views_handler_filter::value_form()
  */
 function value_form(&$form, &$form_state) {
 	$form['value'] = array(
 	  '#type' => 'textfield',
 	  '#title' => t('Value'),
 	  '#size' => 30,
 	  '#default_value' => $this->value,
 	);
 	if (!empty($form_state['exposed'])) {
 		$identifier = $this->options['expose']['identifier'];
 	  if (!empty($form_state['exposed']) && !isset($form_state['input'][$identifier])) {
      $form_state['input'][$identifier] = $this->value;
    }
 	}
 }
 
 /**
  * Extend.
  * @see views_handler_filter::query()
  */
 
 function query() {
 	 while (is_array($this->value)) {
 	 	 $this->value = $this->value ? array_shift($this->value) : NULL;
 	 }
 	 $fields = $this->options['fields'] ? $this->options['fields'] : array_keys($this->getFulltextFields());
 	 
   $entity_id = array('-1');
   $query = search_api_index_load($this->definition['search_api_index'])->query(array('parse mode' => 'terms'));
   
   // Deep copy from search_api
   if ($this->value) {
     while (is_array($this->value)) {
       $this->value = $this->value ? array_shift($this->value) : NULL;
     }
     
    $filter = $this->options['mode'] == 'filter';
    if (!$filter) {
      $old = $query->getFields();
      $filter = $old && (array_diff($old, $fields) || array_diff($fields, $old));
    }

    if ($filter) {
      $filter = $query->createFilter('OR');
      foreach ($fields as $field) {
        $filter->condition($field, $this->value, $this->operator);
      }
      $query->filter($filter);
      return;
    }

    $query->fields($fields);
    $old = $query->getOriginalKeys();
    $query->keys($this->value);
    if ($this->operator != '=') {
      $keys = &$query->getKeys();
      if (is_array($keys)) {
        $keys['#negation'] = TRUE;
      }
      else {
        // We can't know how negation is expressed in the server's syntax.
      }
    }
    if ($old) {
      $keys = &$query->getKeys();
      if (is_array($keys)) {
        $keys[] = $old;
      }
      elseif (is_array($old)) {
        // We don't support such nonsense.
      }
      else {
        $keys = "($old) ($keys)";
      }
    }
   }

   $filters = $this->search_api_views_filter_merge_search_api_filters($this->view, search_api_index_load($this->definition['search_api_index']));
   $search_api_filter = $query->createFilter("OR");
   $filter_value_handlers = module_invoke_all('views_filter_value');
   foreach ($filters as $filter) {
     if (is_subclass_of($filter, 'views_handler_filter_date')) {
       $search_api_filter->condition($filter->real_field, views_handler_filter_date_helper::getValue($filter), $filter->operator);
     }
     elseif (get_class($filter) == 'views_handler_filter_node_type') {
       foreach ($filter->value as $v) {
         $search_api_filter->condition($filter->real_field, $v, '=');
       }
     }
     else {
       $search_api_filter->condition($filter->real_field, $filter->value, $filter->operator);
     }
   }
   $query->filter($search_api_filter);

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
   $this->ensure_my_table();
   $this->query->add_where($this->options['group'], "$this->table_alias.$this->real_field", $entity_id, 'in');
 }
 
 /**
  * Helper function to fetch fields.
  * Steal from search_api_views.
  */
  protected function getFulltextFields() {
    $fields = array();
    $index = search_api_index_load($this->definition['search_api_index']);
    if (!empty($index->options['fields'])) {
      $f = $index->options['fields'];
      foreach ($index->getFulltextFields() as $name) {
        $fields[$name] = $f[$name]['name'];
      }
    }
    return $fields;
  }
  
  /**
   * Add addition expose form.
   * @see views_handler_filter::expose_form()
   */
  function expose_form(&$form, &$form_state) {
  	parent::expose_form($form, $form_state);
  	$form['mode'] = array(
  	      '#title' => t('Use as'),
  	      '#type' => 'radios',
  	      '#options' => array(
  	        'keys' => t('Search keys – multiple words will be split and the filter will influence relevance.'),
  	        'filter' => t("Search filter – use as a single phrase that restricts the result set but doesn't influence relevance."),
  	),
  	       '#default_value' => $this->options['mode'],
  	);
  	$fields = $this->getFulltextFields();
  	if (!empty($fields)) {
  	  $form['fields'] = array(
  	    '#type' => 'select',
  	    '#title' => t('Searched fields'),
  	    '#description' => t('Select the fields that will be searched. If no fields are selected, all available fulltext fields will be searched.'),
  	    '#options' => $fields,
  	    '#size' => min(4, count($fields)),
  	    '#multiple' => TRUE,
  	    '#default_value' => $this->options['fields'],
  	  );
  	}
  	else {
  	  $form['fields'] = array(
  	    '#type' => 'value',
  	    '#value' => array(),
      );
  	}
  }
  
  /**
   * Merge views filter with search_api index fields.
   */
  function search_api_views_filter_merge_search_api_filters($view, $index) {
    $filters = array();
    unset($view->filter[$this->options['id']]);
    foreach ($view->filter as $filter) {
      if (isset($index->options['fields'][$filter->real_field]) && $index->options['fields'][$filter->real_field]['indexed']) {
        $filters[$filter->real_field] = $filter;
      }
    }
    return $filters;
  }
}