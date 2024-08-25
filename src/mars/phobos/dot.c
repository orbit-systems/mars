#include "common/orbit.h"
#include "mars/term.h"
#include "ast.h"
#include "dot.h"

int dot_uID();

void emit_dot(string path, da(AST) nodes) {
	char* path_cstr = clone_to_cstring(path);
	char buffer[1050];
	sprintf(buffer, "%.1024s.dot", path_cstr); //capped at 1024 characters for file name
	mars_free(path_cstr);

	string filename = str(buffer);
	if (fs_exists(filename)) {
		//we delete the file and recreate a new one
		fs_file file;
		fs_get(filename, &file);
		if (fs_delete(&file) == 0) general_error("Failed to delete file: \"%s\"", filename);
	}
	//the file does not exist
	fs_file file;
	fs_create(filename, oft_regular, &file);
	fs_open(&file, "w");
	printf("emitting dot file: \"%s\"\n", filename.raw);

	sprintf(buffer, "digraph \"%d\" {\n\trankdir=\"LR\"\n\tnodesep=0.4\n", dot_uID());
	fs_write(&file, buffer, strlen(buffer));

	for_urange(i, 0, nodes.len) {
		recurse_dot(nodes.at[i], &file, 1, dot_uID());
	}

	fs_write(&file, "}\n", 2);

	fs_close(&file);
	fs_drop(&file);
}

int _dot_uID = 0;

int dot_uID() {
	return ++_dot_uID;
}

void recurse_dot(AST node, fs_file* file, int n, int uid) {
	//this code keeps track of depth with n, in which case it prints that many tabs.
	//all nodes have label sugar to allow them to change names to un-mutated names.
	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	char buffer[4096];

	if (is_null_AST(node)) {
		printf("encountered null node at uid %d\n", uid);
		sprintf(buffer, "\"%s_%d\" [fontcolor=white,shape=diamond, style=filled, label=\"%s\", color=\"1.0 .7 .7\"]\n", 
				ast_type_str[node.type], uid, 
				"NULL\\nAST"); //invalid sugar
	    fs_write(file, buffer, strlen(buffer));
		return;
	}

	switch (node.type) {
		case AST_invalid:
			return;
		case AST_identifier: {

			int int_uid = dot_uID();

	        sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=\".7 .3 1.0\", label=\"%s\"]\n", 
	        		clone_to_cstring(node.as_identifier->tok->text), 
	        		int_uid, 
	        		clone_to_cstring(node.as_identifier->tok->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" [label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier with sugared name
	    	fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_identifier->tok->text), int_uid); //print node link
	        fs_write(file, buffer, strlen(buffer));
	        break;
		}

		case AST_decl_stmt: {
	    	char* decl_type = "";
	        if (node.as_decl_stmt->is_mut) 
	        	decl_type = "mut decl";
	        else 
	        	decl_type = "let decl"; //determine name

	        sprintf(buffer, "\"%s_%d\" [label=\"%s\"]\n", "declaration", uid, decl_type); //decl sugar
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        for_urange(i, 0, node.as_decl_stmt->lhs.len) { //iterate over function itself
	        	int int_uid = dot_uID();
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", "declaration", uid, ast_type_str[node.as_decl_stmt->lhs.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	            recurse_dot(node.as_decl_stmt->lhs.at[i], file, n+1, int_uid);
	        }
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		"declaration", uid, 
	        		ast_type_str[node.as_decl_stmt->type.type], _dot_uID + 1); //link
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_decl_stmt->type, file, n+1, dot_uID()); //print type info

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		"declaration", uid, 
	        		ast_type_str[node.as_decl_stmt->rhs.type], _dot_uID + 1); //print links
	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_decl_stmt->rhs, file, n+1, dot_uID());
			break;
		}

		case AST_func_literal_expr: {
	        sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=green, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs			

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->type.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->type, file, n+1, dot_uID());
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_func_literal_expr->code_block.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_func_literal_expr->code_block, file, n+1, dot_uID());
	    	break;
	    }

		case AST_fn_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=green, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));
			for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); //fix tabs	
	        
	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "label=parameters\n");
	        fs_write(file, buffer, strlen(buffer));

	        for_urange(i, 0, node.as_fn_type_expr->parameters.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_fn_type_expr->parameters.at[i].type.type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_fn_type_expr->parameters.at[i].type, file, n+2, int_uid);
	        }



	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);		
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));
			for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); //fix tabs	
	        
	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "label=returns\n");
	        fs_write(file, buffer, strlen(buffer));


	    	for_urange(i, 0, node.as_fn_type_expr->returns.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", ast_type_str[node.type], uid, ast_type_str[node.as_fn_type_expr->returns.at[i].type.type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_fn_type_expr->returns.at[i].type, file, n+2, int_uid);
	        }

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}", 1);	
			break;
		}

		case AST_basic_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

			sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"", 
					ast_type_str[node.type], uid, 
					clone_to_cstring(node.as_basic_type_expr->lit->text), _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" [shape=house,label=\"%s\"]", 
	        		clone_to_cstring(node.as_basic_type_expr->lit->text), _dot_uID + 1,
	        		clone_to_cstring(node.as_basic_type_expr->lit->text));
	        fs_write(file, buffer, strlen(buffer));
	    	
	    	for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	    	break;
		}

		case AST_stmt_block: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        for_urange(i, 0, node.as_stmt_block->stmts.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_stmt_block->stmts.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_stmt_block->stmts.at[i], file, n+1, int_uid);
	        }
	        break;
		}

		case AST_cast_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	
	    	
	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_cast_expr->type.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_cast_expr->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_cast_expr->type, file, n+1, int_uid);
	    	recurse_dot(node.as_cast_expr->rhs, file, n+1, int_uid + 1);

			break;
		}

		case AST_pointer_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_pointer_type_expr->subexpr.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_pointer_type_expr->subexpr, file, n+1, int_uid);

	        break;
		}

		case AST_binary_op_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"binary op %s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_binary_op_expr->op->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
			_dot_uID += 1;
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_binary_op_expr->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_binary_op_expr->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_binary_op_expr->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_binary_op_expr->rhs, file, n+1, int_uid + 1);

			break;
		}

		case AST_while_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();

	    	_dot_uID += 1;
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_while_stmt->condition.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_while_stmt->block.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=condition\n");
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));         	

	        recurse_dot(node.as_while_stmt->condition, file, n+1, int_uid);

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	    	recurse_dot(node.as_while_stmt->block, file, n+1, int_uid + 1);

	        break;
		}

		case AST_assign_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        for_urange(i, 0, node.as_assign_stmt->lhs.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_assign_stmt->lhs.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_assign_stmt->lhs.at[i], file, n+1, int_uid);
	        }

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_assign_stmt->rhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 

	        recurse_dot(node.as_assign_stmt->rhs, file, n+1, int_uid);

	        break;
		}

		case AST_unary_op_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"unary op %s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		clone_to_cstring(node.as_unary_op_expr->op->text)); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	       	int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_unary_op_expr->inside.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_unary_op_expr->inside, file, n+1, int_uid);

	        break;
		}

		case AST_literal_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        char* literal_name = "invalid literal type";

	    	char literal_buffer[2048];

	        //determine what type of literal, and then emit the correct node.
	        switch (node.as_literal_expr->value.kind) {
	        	case EV_BOOL: {
	        		literal_name = "bool";
	        		sprintf(literal_buffer, "%s", node.as_literal_expr->value.as_bool == true ? "true" : "false");
	        		break;
	        	}
	        	case EV_STRING: {
	        		literal_name = "string";
	        		sprintf(literal_buffer, "%.1024s", clone_to_cstring(node.as_literal_expr->value.as_string));
	        		break;
	        	}
	        	case EV_UNTYPED_INT: {
	        		literal_name = "int";
	        		sprintf(literal_buffer, "%d", node.as_literal_expr->value.as_untyped_int);
	        		break;
	        	}
	        	case EV_UNTYPED_FLOAT: {
	        		literal_name = "float";
	        		sprintf(literal_buffer, "%lf", node.as_literal_expr->value.as_untyped_float);
	        		break;
	        	}
	        	case EV_POINTER: {
	        		literal_name = "pointer";
	        		sprintf(literal_buffer, "null");
	        		break;
	        	}
	        	default:
	        }

	        int int_uid = dot_uID();

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		literal_name, int_uid);
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); 
	       	sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue,label=\"%s\\n%s\"]\n", 
	        		literal_name, int_uid, 
	        		literal_name, literal_buffer);
	        fs_write(file, buffer, strlen(buffer));

	       	break;
		}

		case AST_selector_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_selector_expr->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_selector_expr->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_selector_expr->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_selector_expr->rhs, file, n+1, int_uid + 1);
	    	break;
		}

		case AST_import_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_import_stmt->name.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_import_stmt->path.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_import_stmt->name, file, n+1, int_uid);
	    	recurse_dot(node.as_import_stmt->path, file, n+1, int_uid + 1);
	    	break;
		}

		case AST_if_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        
	    	_dot_uID += 3;

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_if_stmt->condition.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_if_stmt->if_branch.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=condition\n");
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));         	

	        recurse_dot(node.as_if_stmt->condition, file, n+1, int_uid);

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	    	sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=\"if branch\"\n");
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));         	

	        recurse_dot(node.as_if_stmt->if_branch, file, n+1, int_uid + 1);

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	    	sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=\"else branch\"\n");
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));         	

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	        break;
		}

		case AST_call_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        int int_uid = dot_uID();

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_call_expr->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_call_expr->lhs, file, n+1, int_uid);

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "label=params\n");
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));

	        for_urange(i, 0, node.as_call_expr->params.len) {
	        	int_uid = dot_uID() + 1;
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_call_expr->params.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_call_expr->params.at[i], file, n+1, int_uid);
	        }

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);

	        break;	
		}

		case AST_expr_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_expr_stmt->expression.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_expr_stmt->expression, file, n+1, dot_uID());

	        break;
		}

		case AST_for_stmt: {
			general_warning("fuck! shit arse");
			/*
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

			int int_uid = dot_uID();
			_dot_uID += 4;
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_stmt->prelude.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_stmt->condition.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_stmt->update.type], int_uid + 2);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_stmt->block.type], int_uid + 3);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_for_stmt->prelude, file, n+1, int_uid);
	    	recurse_dot(node.as_for_stmt->condition, file, n+1, int_uid + 1);
			recurse_dot(node.as_for_stmt->update, file, n+1, int_uid + 2);
	    	recurse_dot(node.as_for_stmt->block, file, n+1, int_uid + 3);
			*/
	    	break;
		}

		case AST_comp_literal_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        for_urange(i, 0, node.as_comp_literal_expr->elems.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_comp_literal_expr->elems.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer)); 
	        	recurse_dot(node.as_comp_literal_expr->elems.at[i], file, n+1, int_uid);
	        }

	        int int_uid = dot_uID();

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_comp_literal_expr->type.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_comp_literal_expr->type, file, n+1, int_uid);

	        break;
		}

		case AST_for_in_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

			int int_uid = dot_uID();
			_dot_uID += 5;
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_in_stmt->indexvar.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_in_stmt->type.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_in_stmt->to.type], int_uid + 2);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_in_stmt->from.type], int_uid + 3);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_for_in_stmt->block.type], int_uid + 4);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_for_in_stmt->indexvar, file, n+1, int_uid);
	    	recurse_dot(node.as_for_in_stmt->type, file, n+1, int_uid + 1);
			recurse_dot(node.as_for_in_stmt->to, file, n+1, int_uid + 2);
	    	recurse_dot(node.as_for_in_stmt->from, file, n+1, int_uid + 3);
	    	recurse_dot(node.as_for_in_stmt->block, file, n+1, int_uid + 4);

	    	break;
		}

		case AST_slice_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_slice_type_expr->subexpr.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_slice_type_expr->subexpr, file, n+1, int_uid);

	        break;
		}

		case AST_slice_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

			int int_uid = dot_uID();
			_dot_uID += 3;
	        
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_slice_expr->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_slice_expr->inside_left.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_slice_expr->inside_right.type], int_uid + 2);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);


	        recurse_dot(node.as_slice_expr->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_slice_expr->inside_left, file, n+1, int_uid + 1);
			recurse_dot(node.as_slice_expr->inside_right, file, n+1, int_uid + 2);

	    	break;
		}
		
		case AST_type_decl_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        int int_uid = dot_uID();
	        _dot_uID += 1;

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_type_decl_stmt->lhs.type], int_uid);
	        fs_write(file, buffer, strlen(buffer)); 
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_type_decl_stmt->rhs.type], int_uid + 1);
	        fs_write(file, buffer, strlen(buffer)); 
	        //for (int i = 0; i < n; i++) fs_write(file, "\t", 1);

	        recurse_dot(node.as_type_decl_stmt->lhs, file, n+1, int_uid);
	    	recurse_dot(node.as_type_decl_stmt->rhs, file, n+1, int_uid + 1);
	    	break;
		}

		case AST_struct_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs	

	        sprintf(buffer, "subgraph cluster%d {\n", dot_uID());
	        fs_write(file, buffer, strlen(buffer));
			for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1); //fix tabs	
	        
	        sprintf(buffer, "color=red\n");
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < (n + 1); i++) fs_write(file, "\t", 1);

	       	sprintf(buffer, "label=fields\n");
	        fs_write(file, buffer, strlen(buffer));

	        for_urange(i, 0, node.as_struct_type_expr->fields.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_struct_type_expr->fields.at[i].type.type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_struct_type_expr->fields.at[i].type, file, n+2, int_uid);
	        }

	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1);
	        fs_write(file, "}\n", 2);		

	        break;
		}

		case AST_distinct_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=pink, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.as_distinct_type_expr->subexpr.type], _dot_uID + 1);
	        fs_write(file, buffer, strlen(buffer));
	    	recurse_dot(node.as_distinct_type_expr->subexpr, file, n+1, dot_uID());

	        break;
		}

		case AST_return_stmt: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs

	        for_urange(i, 0, node.as_return_stmt->returns.len) {
	        	int int_uid = dot_uID();
	        	for (int i = 0; i < (n+1); i++) fs_write(file, "\t", 1);
	        	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_return_stmt->returns.at[i].type], int_uid);
	            fs_write(file, buffer, strlen(buffer));
	        	recurse_dot(node.as_return_stmt->returns.at[i], file, n+1, int_uid);
	        }
	        break;
		}

		case AST_array_type_expr: {
			sprintf(buffer, "\"%s_%d\" [shape=box,style=filled,color=lightblue, label=\"%s\"]\n", 
	        		ast_type_str[node.type], uid, 
	        		ast_type_str[node.type]); //write out identifier_expr with sugared name
	        fs_write(file, buffer, strlen(buffer));
	        for (int i = 0; i < n; i++) fs_write(file, "\t", 1); //fix tabs
	        int int_uid = dot_uID();
	        sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_array_type_expr->length.type], int_uid);

	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_array_type_expr->length, file, n+1, int_uid);
	        int_uid = dot_uID();
	       	sprintf(buffer, "\"%s_%d\" -> \"%s_%d\"\n", 
	        			ast_type_str[node.type], uid, 
	        			ast_type_str[node.as_array_type_expr->type.type], int_uid);

	        fs_write(file, buffer, strlen(buffer));
	        recurse_dot(node.as_array_type_expr->type, file, n+1, int_uid);
	        break;
		}

		default: {
			sprintf(buffer, "\"unimpl_%d\"->\"unimpl_%d\" unimpl_%d [shape=star,style=filled,color=yellow]", uid, uid, uid);
			fs_write(file, buffer, strlen(buffer));
			
			if (node.type < AST_COUNT) {
				printf("encountered unimplemented node: %s\n", ast_type_str[node.type]);
			} else {
				printf("unimpl: %d\n", node.type);
			}
			
			break;
		}

	}

	fs_write(file, "\n", 1);
}